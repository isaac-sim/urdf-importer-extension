URDF Import Extension [omni.importer.urdf]
##########################################



URDF Import Commands
====================
The following commands can be used to simplify the import process.
Below is a sample demonstrating how to import the Carter URDF included with this extension

.. code-block:: python
    :linenos:

    import omni.kit.commands
    from pxr import UsdLux, Sdf, Gf, UsdPhysics, PhysicsSchemaTools

    # setting up import configuration:
    status, import_config = omni.kit.commands.execute("URDFCreateImportConfig")
    import_config.merge_fixed_joints = False
    import_config.convex_decomp = False
    import_config.import_inertia_tensor = True
    import_config.fix_base = False
    import_config.collision_from_visuals = False

    # Get path to extension data:
    ext_manager = omni.kit.app.get_app().get_extension_manager()
    ext_id = ext_manager.get_enabled_extension_id("omni.importer.urdf")
    extension_path = ext_manager.get_extension_path(ext_id)
    # import URDF
    omni.kit.commands.execute(
        "URDFParseAndImportFile",
        urdf_path=extension_path + "/data/urdf/robots/carter/urdf/carter.urdf",
        import_config=import_config,
    )
    # get stage handle
    stage = omni.usd.get_context().get_stage()

    # enable physics
    scene = UsdPhysics.Scene.Define(stage, Sdf.Path("/physicsScene"))
    # set gravity
    scene.CreateGravityDirectionAttr().Set(Gf.Vec3f(0.0, 0.0, -1.0))
    scene.CreateGravityMagnitudeAttr().Set(9.81)

    # add ground plane
    PhysicsSchemaTools.addGroundPlane(stage, "/World/groundPlane", "Z", 1500, Gf.Vec3f(0, 0, -50), Gf.Vec3f(0.5))

    # add lighting
    distantLight = UsdLux.DistantLight.Define(stage, Sdf.Path("/DistantLight"))
    distantLight.CreateIntensityAttr(500)

.. automodule:: omni.importer.urdf.scripts.commands
    :members:
    :undoc-members:
    :exclude-members: do, undo

.. automodule:: omni.importer.urdf._urdf

.. autoclass:: omni.importer.urdf._urdf.Urdf
    :members:
    :undoc-members:
    :no-show-inheritance:

.. autoclass:: omni.importer.urdf._urdf.ImportConfig
    :members:
    :undoc-members:
    :no-show-inheritance:
