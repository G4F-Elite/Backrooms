"""Check the English dispatcher bank and every shipped, prerecorded voice line."""

from array import array
from collections import Counter
from pathlib import Path
import re
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
BANK = ROOT / "src" / "dispatcher_lines.inc"
CLIPS = ROOT / "assets" / "dispatcher"
LINE = re.compile(r'DISPATCH_LINE\(([a-z][a-z_0-9]*), ([a-z][a-z_0-9]*), "([^"]+)"\)')


class DispatcherAssetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lines = []
        for number, line in enumerate(BANK.read_text(encoding="ascii").splitlines(), 1):
            match = LINE.fullmatch(line)
            if match is None:
                raise ValueError(f"{BANK}:{number}: malformed dispatcher entry")
            cls.lines.append(match.groups())

    def test_each_event_has_multiple_distinct_readings(self):
        counts = Counter(event for event, _, _ in self.lines)
        self.assertGreaterEqual(len(counts), 46)
        self.assertGreaterEqual(counts["long_walk"], 14)
        for event, count in counts.items():
            with self.subTest(event=event):
                self.assertGreaterEqual(count, 5)
        subtitles = [subtitle.casefold() for _, _, subtitle in self.lines]
        self.assertEqual(len(subtitles), len(set(subtitles)), "repeated subtitle")

    def test_bank_and_shipped_files_correspond_one_to_one(self):
        names = [clip + ".opus" for _, clip, _ in self.lines]
        self.assertEqual(len(names), len(set(names)), "reused clip identifier")
        self.assertSetEqual(set(names), {path.name for path in CLIPS.iterdir() if path.is_file()})

    def test_narrator_observes_without_addressing_the_player(self):
        speaker_or_listener = re.compile(
            r"\b(?:I|me|my|mine|myself|we|us|our|ours|ourselves|you|your|yours|yourself|yourselves)\b",
            re.IGNORECASE,
        )
        for _, clip, subtitle in self.lines:
            with self.subTest(clip=clip):
                self.assertNotRegex(subtitle, speaker_or_listener)

    def test_each_clip_contains_audible_complete_mono_opus(self):
        for _, clip, _ in self.lines:
            with self.subTest(clip=clip):
                path = CLIPS / (clip + ".opus")
                data = path.read_bytes()
                offset = 0
                while offset < len(data):
                    header = data[offset:offset + 27]
                    self.assertEqual(len(header), 27)
                    self.assertEqual(header[:5], b"OggS\x00")
                    payload = offset + 27 + header[26]
                    self.assertLessEqual(payload, len(data))
                    if offset == 0:
                        self.assertEqual(data[payload:payload + 10], b"OpusHead\x01\x01")
                        pre_skip = int.from_bytes(data[payload + 10:payload + 12], "little")
                    offset = payload + sum(data[offset + 27:payload])
                    self.assertLessEqual(offset, len(data))
                    self.assertEqual(bool(header[5] & 4), offset == len(data), "misplaced or missing EOS")
                frames = int.from_bytes(header[6:14], "little") - pre_skip
                self.assertGreaterEqual(frames, 24000)
                pcm = subprocess.check_output([
                    "ffmpeg", "-nostdin", "-v", "error", "-xerror", "-i", str(path),
                    "-map", "0:a:0", "-ar", "48000", "-f", "s16le", "pipe:1",
                ])
                self.assertEqual(len(pcm), frames * 2)
                samples = array("h", pcm)
                if sys.byteorder == "big":
                    samples.byteswap()
                voiced = sum(abs(sample) >= 256 for sample in samples)
                self.assertGreater(voiced, frames // 100, "clip is effectively silent")


if __name__ == "__main__":
    unittest.main()
