# Usage

To enable this extension, go to the Extension Manager menu and enable isaacsim.asset.importer.urdf extension.

# High Level Code Overview

## Python
The URDF Importer extension sets attributes of `ImportConfig` on initialization,
along with the UI giving the user options to change certain attributes.  The complete list of configs
can be found in `bindings/BindingsUrdfPython.cpp`.

When the import button is pressed, the `URDFParseAndImportFile` command from `python/commands.py` is invoked; this first calls `URDFParseFile` command, which binds to `parseUrdf` in `plugins/Urdf.cpp`. It then runs `import_robot` which binds to `importRobot` in `plugins/Urdf.cpp`.


## C++
### `parseUrdf` in `plugins/Urdf.cpp`
Calls `parseUrdf` in `plugins/UrdfParser.cpp`, which parses the URDF file using tinyxml2 and calls `parseRobot` in the same file. `parseRobot` writes to the `urdfRobot` class, which is an the internal data structure that contains the name, the links, the joints, and the materials of the robot. It first parses materials, which are stored in a map between material name and `UrdfMaterial` class that contains the material name, the color, and the texture file path. It then parses the links, which are stored in a map between the link name and `UrdfLink` class containing the name, the inertial, visual mesh, and collision mesh. It finally parses the joints and store each joint in `UrdfJoint`. Note that for joints, if there is no axis specified, the axis defaults to (1,0,0). If there is no origin, it defaults to the identity transformation.


### `importRobot` in `plugins/Urdf.cpp`
Initializes `urdfImporter` class defined in `plugins/UrdfImporter.h`. Calls `urdfImporter.addToStage`, which does the following:
- Creates a physics scene if desired in the config
- Creates a new prim that represents the robot and applies Articulation API, enables self-collision is set in the config, and sets the position and velocity iteraiton count to 32 and 16 by default.
- Sets the robot prim as the default prim if desired in the config
- Initializes a `KinematicChain` object from `plugins/KinematicChain.h` and calls `computeKinematicChain` given the `urdfRobot`. `computeKinematicChain` first finds the base link and calls `computeChildNodes` recursively to populate the entire kinematic chain.
- All Meshes are added to a Mesh scope, and referenced where necessary. Visual and colliders are also created on a separate Scope and referenced in the robot, optimizing memory usage when the same mesh is used in multiple instances, as well as when the robot is duplicated on scene.
- Recursively calls `addLinksAndJoints` along the entire kinematic chain to add the link and joint prims on stage.



# Limitations
- URDFs by definition only support kinematic trees. To overcome this limitation in looped articulations, a custom loop_joint tag is implemented. This joint does not add drives or limits, so the user needs to pick an adequate joint in the loop to convert in a loop_joint.
- The URDF importer does not currently support the `planar` joint type. In order to achieve a planar movement, please define two prismatic joints in tandem in place of a planar joint.