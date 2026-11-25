"""Check the English dispatcher bank and every shipped, prerecorded voice line."""

from array import array
from collections import Counter
from pathlib import Path
import re
import sys
import unittest
import wave


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
        names = [clip + ".wav" for _, clip, _ in self.lines]
        self.assertEqual(len(names), len(set(names)), "reused clip identifier")
        self.assertSetEqual(set(names), {path.name for path in CLIPS.glob("*.wav")})

    def test_narrator_observes_without_addressing_the_player(self):
        speaker_or_listener = re.compile(
            r"\b(?:I|me|my|mine|myself|we|us|our|ours|ourselves|you|your|yours|yourself|yourselves)\b",
            re.IGNORECASE,
        )
        for _, clip, subtitle in self.lines:
            with self.subTest(clip=clip):
                self.assertNotRegex(subtitle, speaker_or_listener)

    def test_each_clip_contains_audible_complete_mono_pcm(self):
        for _, clip, _ in self.lines:
            with self.subTest(clip=clip), wave.open(str(CLIPS / (clip + ".wav")), "rb") as audio:
                self.assertEqual(audio.getnchannels(), 1)
                self.assertEqual(audio.getsampwidth(), 2)
                self.assertEqual(audio.getframerate(), 16000)
                self.assertEqual(audio.getcomptype(), "NONE")
                frames = audio.getnframes()
                self.assertGreaterEqual(frames, audio.getframerate() // 2)
                pcm = audio.readframes(frames)
                self.assertEqual(len(pcm), frames * 2)
                samples = array("h", pcm)
                if sys.byteorder == "big":
                    samples.byteswap()
                voiced = sum(abs(sample) >= 256 for sample in samples)
                self.assertGreater(voiced, frames // 100, "clip is effectively silent")


if __name__ == "__main__":
    unittest.main()
