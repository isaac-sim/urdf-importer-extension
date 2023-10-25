# Omniverse URDF Importer

The URDF Importer Extension is used to import URDF representations of robots.
`Unified Robot Description Format (URDF)` is an XML format for representing a robot model in ROS.

## Getting Started

1. Clone the GitHub repo to your local machine.
2. Open a command prompt and navigate to the root of your cloned repo.
3. Run `build.bat` to bootstrap your dev environment and build the example extensions.
4. Run `_build\{platform}\release\omni.importer.urdf.app.bat` to start the Kit application.
5. From the menu, select `Isaac Utils->URDF Importer` to launch the UI for the URDF Importer extension.

This extension is enabled by default. If it is ever disabled, it can be re-enabled from the Extension Manager
by searching for `omni.importer.urdf`.

**Note:** On Linux, replace `.bat` with `.sh` in the instructions above.


## Conventions

Special characters in link or joint names are not supported and will be replaced with an underscore. In the event that the name starts with an underscore due to the replacement, an a is pre-pended. It is recommended to make these name changes in the mjcf directly.

See the [Convention References](https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/reference_conventions.html#isaac-sim-conventions) documentation for a complete list of `Isaac Sim` conventions.


## User Interface
![URDF Importer UI](/images/urdf_importer_ui.png)

1. Information Panel: This panel has useful information about this extension.
2. Import Options Panel: This panel has utility functions for testing the gains being set for the Articulation.
See `Import Options` below for full details.
3. Import Panel: This panel holds the source path, destination path, and import button.


## Import Options

* **Merge Fixed Joints**: Consolidate links that are connected by fixed joints, so that an articulation is only applied to joints that move.
* **Fix Base Link**: When checked, the robot will have its base fixed where it's placed in world coordinates.
* **Import Inertia Tensor**: Check to load inertia from urdf directly. If the urdf does not specify an inertia tensor, identity will be used and scaled by the scaling factor. If unchecked, Physx will compute it automatically.
* **Stage Units Per Meter**: |kit| default length unit is centimeters. Here you can set the scaling factor to match the unit used in your URDF. Currently, the URDF importer only supports uniform global scaling. Applying different scaling for different axes and specific mesh parts (i.e. using the ``scale`` parameter under the URDF mesh label) will be available in future releases. If you have a ``scale`` parameter in your URDF, you may need to manually adjust the other values in the URDF so that all parameters are in the same unit.
* **Link Density**: If a link does not have a given mass, uses this density (in Kg/m^3) to compute mass based on link volume. A value of 0.0 can be used to tell the physics engine to automatically compute density as well.
* **Joint Drive Type**: Default Joint drive type, Values can be `None`, `Position`, and `Velocity`.
* **Joint Drive Strength**: The drive strength is the joint stiffness for position drive, or damping for velocity driven joints.
* **Joint Position Drive Damping**: If the drive type is set to position this is the default damping value used.
* **Clear Stage**: When checked, cleans the stage before loading the new URDF, otherwise loads it on current open stage at position `(0,0,0)`
* **Convex Decomposition**: If Checked, the collision object will be made a set of Convex meshes to better match the visual asset. Otherwise a convex hull will be used.
* **Self Collision**: Enables self collision between adjacent links. It may cause instability if the collision meshes are intersecting at the joint.
* **Create Physics Scene**: Creates a default physics scene on the stage. Because this physics scene is created outside of the robot asset, it won't be loaded into other scenes composed with the robot asset.
* **Output Directory**: The destination of the imported asset. it will create a folder structure with the robot asset and all textures used for its rendering. You must have write access to this directory.

**Note:**
- It is recommended to set Self Collision to false unless you are certain that links on the robot are not self colliding.
- You must have write access to the output directory used for import, it will default to the current open stage, change this as necessary.

**Known Issue:** If more than one asset in URDF contains the same material name, only one material will be created, regardless if the parameters in the material are different (e.g two meshes have materials with the name "material", one is blue and the other is red. both meshes will be either red or blue.). This also applies for textured materials.



## Robot Properties

There might be many properties you want to tune on your robot. These properties can be spread across many different Schemas and APIs.

The general steps of getting and setting a parameter are:

1. Find which API is the parameter under. Most common ones can be found in the [Pixar USD API](https://docs.omniverse.nvidia.com/kit/docs/kit-manual/latest/api/pxr_index.html).

2. Get the prim handle that the API is applied to. For example, Articulation and Drive APIs are applied to joints, and MassAPIs are applied to the rigid bodies.

3. Get the handle to the API. From there on, you can Get or Set the attributes associated with that API.

For example, if we want to set the wheel's drive velocity and the actuators' stiffness, we need to find the DriveAPI:


```python
    # get handle to the Drive API for both wheels
    left_wheel_drive = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/carter/chassis_link/left_wheel"), "angular")
    right_wheel_drive = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/carter/chassis_link/right_wheel"), "angular")

    # Set the velocity drive target in degrees/second
    left_wheel_drive.GetTargetVelocityAttr().Set(150)
    right_wheel_drive.GetTargetVelocityAttr().Set(150)

    # Set the drive damping, which controls the strength of the velocity drive
    left_wheel_drive.GetDampingAttr().Set(15000)
    right_wheel_drive.GetDampingAttr().Set(15000)

    # Set the drive stiffness, which controls the strength of the position drive
    # In this case because we want to do velocity control this should be set to zero
    left_wheel_drive.GetStiffnessAttr().Set(0)
    right_wheel_drive.GetStiffnessAttr().Set(0)
```

Alternatively you can use the [Omniverse Commands Tool](https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/ext_omni_kit_commands.html#isaac-sim-command-tool) to change a value in the UI and get the associated Omniverse command that changes the property.

**Note:**
    - The drive stiffness parameter should be set when using position control on a joint drive.
    - The drive damping parameter should be set when using velocity control on a joint drive.
    - A combination of setting stiffness and damping on a drive will result in both targets being applied, this can be useful in position control to reduce vibrations.


## Examples

The following examples showcase how to best use this extension:
- Carter Example: `Isaac Examples > Import Robot > Carter URDF`
- Franka Example: `Isaac Examples > Import Robot > Franka URDF`
- Kaya Example: `Isaac Examples > Import Robot > Kaya URDF`
- UR10 Example: `Isaac Examples > Import Robot > UR10 URDF`

**Note:** For these example, please wait for materials to get loaded. You can track progress on the bottom right corner of UI.

### Carter URDF Example

To run the Example:
- Go to the top menu bar and click `Isaac Examples > Import Robots > Carter URDF`.
- Press the ``Load Robot`` button to import the URDF into the stage, add a ground plane, add a light, and and a physics scene.
- Press the ``Configure Drives`` button to to configure the joint drives and allow the rear pivot to spin freely.
- Press the `Open Source Code` button to view the source code. The source code illustrates how to import and integrate the robot using the Python API.
- Press the ``PLAY`` button to begin simulating.
- Press the ``Move to Pose`` button to make the robot drive forward.

![Carter URDF Example](/images/urdf_carter.png)


### Franka URDF Example

To run the Example:
- Go to the top menu bar and click `Isaac Examples > Import Robots > Franka URDF`.
- Press the ``Load Robot`` button to import the URDF into the stage, add a ground plane, add a light, and and a physics scene.
- Press the ``Configure Drives`` button to to configure the joint drives. This sets each drive stiffness and damping value.
- Press the `Open Source Code` button to view the source code. The source code illustrates how to import and integrate the robot using the Python API.
- Press the ``PLAY`` button to begin simulating.
- Press the ``Move to Pose`` button to make the robot move to a home/rest position.

![Franka URDF Example](/images/urdf_franka.png)


### Kaya URDF Example

To run the Example:
- Go to the top menu bar and click `Isaac Examples > Import Robots > Kaya URDF`.
- Press the ``Load Robot`` button to import the URDF into the stage, add a ground plane, add a light, and and a physics scene.
- Press the ``Configure Drives`` button to to configure the joint drives. This sets the drive stiffness and damping value of each wheel, sets all of its rollers as freely rotating.
- Press the `Open Source Code` button to view the source code. The source code illustrates how to import and integrate the robot using the Python API.
- Press the ``PLAY`` button to begin simulating.
- Press the ``Move to Pose`` button to make the robot rotate in place.

![Kaya URDF Example](/images/urdf_kaya.png)


### UR10 URDF Example

- Go to the top menu bar and click `Isaac Examples > Import Robots > Kaya URDF`.
- Press the ``Load Robot`` button to import the URDF into the stage, add a ground plane, add a light, and and a physics scene.
- Press the ``Configure Drives`` button to to configure the joint drives. This sets each drive stiffness and damping value.
- Press the `Open Source Code` button to view the source code. The source code illustrates how to import and integrate the robot using the Python API.
- Press the ``PLAY`` button to begin simulating.
- Press the ``Move to Pose`` button to make the robot move to a home/rest position.

![UR10 URDF Example](/images/urdf_ur10.png)
