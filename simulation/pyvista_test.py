import pyvista as pv

sphere = pv.Sphere(radius=1.5)
arrow = pv.Arrow(
    start=(0, 0, 0),
    direction=(1.1, 0.7, 0.8),
    scale=1.5,
)

plotter = pv.Plotter()
plotter.add_mesh(sphere, color="blue", opacity=0.25)
plotter.add_mesh(arrow, color="yellow")
plotter.add_axes()
plotter.show()
