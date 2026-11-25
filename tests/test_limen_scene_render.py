"""Opt-in headless OpenGL checks. Install moderngl to run on a GPU test machine."""

from pathlib import Path
import re
import struct
import unittest

try:
    import moderngl
except ImportError:
    moderngl = None


SRC = Path(__file__).resolve().parents[1] / "src"


def embedded_shader(header, name):
    match = re.search(rf'\b{name}\s*=\s*R"\((.*?)\)";', header, re.DOTALL)
    return match.group(1)


@unittest.skipUnless(moderngl, "requires moderngl and an OpenGL 3.3 driver")
class LimenSceneTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.context = moderngl.create_standalone_context(require=330)
        cls.addClassCleanup(cls.context.release)
        cls.header = (SRC / "limen_scene_shader.h").read_text(encoding="utf-8")
        includes = re.search(r"\blmMainFS\s*=\s*(.*?);", cls.header, re.DOTALL).group(1)
        pieces = re.findall(r'#include "(.*?)"', includes)
        cls.fragment = "".join(
            (SRC / name).read_text(encoding="utf-8").strip().removeprefix('R"(').removesuffix(')"')
            for name in pieces
        )

    def test_world_and_lamp_programs_link(self):
        projection = embedded_shader(self.header, "lmQuirkProjGLSL")
        for vertex_name, fragment in [
            ("lmMainVSBody", self.fragment),
            ("lmLightVSBody", embedded_shader(self.header, "lmLightFS")),
        ]:
            with self.subTest(shader=vertex_name):
                vertex = embedded_shader(self.header, vertex_name).replace("//QUIRK_PROJ", projection)
                program = self.context.program(vertex_shader=vertex, fragment_shader=fragment)
                program.release()

    def test_floor_joint_pixel_coverage(self):
        # Use the exact production function and a floating point render target. Each
        # sample below is a world position and the projected width of one pixel.
        vertex = """#version 330
        void main() {
            vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
            gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
        }"""
        fragment = self.fragment.replace("void main(){", "void sceneMain(){") + """
        uniform vec2 center, footprint;
        void main() { F = vec4(lmJointCoverage(center, footprint, 2.0, 0.025)); }
        """
        program = self.context.program(vertex_shader=vertex, fragment_shader=fragment)
        self.addCleanup(program.release)
        target = self.context.simple_framebuffer((1, 1), components=1, dtype="f4")
        self.addCleanup(target.release)
        target.use()
        vao = self.context.vertex_array(program, [])
        self.addCleanup(vao.release)
        cases = [
            ((0, 0), (0.01, 0.01), 1.0),       # inside both lines
            ((0.6, 0.5), (0.01, 0.01), 0.0),   # between lines
            ((-2.025, 0.5), (0.01, 0.01), 0.5), # a negative-world edge
            ((0.1, 0.5), (2.0, 0.01), 0.025),   # one whole period
            ((-7.3, 0.5), (8.0, 0.01), 0.025),  # several periods, shifted
            ((131072, 0.5), (0.002, 0.01), 1.0), # thin footprint far from origin
            ((2.4, -0.9), (4, 4), 1 - 0.975 ** 2),
        ]
        for center, footprint, expected in cases:
            with self.subTest(center=center, footprint=footprint):
                program["center"].value = center
                program["footprint"].value = footprint
                vao.render(vertices=3)
                actual, = struct.unpack("f", target.read(components=1, dtype="f4"))
                self.assertAlmostEqual(actual, expected, places=4)


if __name__ == "__main__":
    unittest.main()
