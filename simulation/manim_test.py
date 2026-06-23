from manim import *


class SphereWithArrow(ThreeDScene):
    def construct(self):
        self.set_camera_orientation(
            phi=65 * DEGREES,
            theta=35 * DEGREES
        )

        sphere = Sphere(
            radius=1.5,
            resolution=(32, 64),
            color=BLUE,
            fill_opacity=0.25,
        )

        arrow = Arrow3D(
            start=[0, 0, 0],
            end=[1.1, 0.7, 0.8],
            color=YELLOW,
            thickness=0.035,
            height=0.25,
            base_radius=0.08,
        )

        axes = ThreeDAxes(
            x_range=[-2, 2, 1],
            y_range=[-2, 2, 1],
            z_range=[-2, 2, 1],
        )

        self.add(axes, sphere, arrow)

        self.begin_ambient_camera_rotation(rate=0.25)
        self.wait(5)
