# Changelog

## [1.2.0] - 2023-11-16

### Fixed
- Fixed setting of articulation root on the actual link prim instead of the default prim. This
  is needed to deal with letting PhysX parser decide what is the root prim for floating-base systems.

## [1.1.4] - 2023-10-18

### Changed
- Update code dependencies

## [1.1.3] - 2023-10-02

### Changed
- Mesh path parser now allows for prefixes other than "package://"

## [1.1.2] - 2023-08-21

### Added
- Added processing of capsule bodies to replace cylinders

### Fixed
- Fixed computation of joint axis. Earlier arbitrary axis were not supported.
- Fixed bug in merging joints when multiple levels of fixed joints exist.

## [1.1.1] - 2023-08-08

### Added
- Added support for the boolean attribute  ``<joint dont_collapse>``: setting this parameter to true in the URDF joint tag prevents the child link from collapsing when the associated joint type is "fixed".

## [1.1.0] - 2023-08-04
### Added
- Suport for direct mimic joints
- Maintain Merged Links as frames inside parent rigid body
### Fixed
- Arbitrary joint axis were adding random 56.7 rotation degrees in the joint axis orientation

## [1.0.0] - 2023-07-03
### Changed
- Renamed the extension to omni.importer.urdf
- Published the extension to the default registry

## [0.5.16] - 2023-06-27
### Fixed
- Support for arbitrary joint axis.

## [0.5.15] - 2023-06-26
### Fixed
- Support for non-diagonal inertia matrix
- Support for convex decomposition mesh collision method

## [0.5.14] - 2023-06-13
### Fixed
- Kit 105.1 update
- Accessing primvars is accessed through the PrimvarsAPI instead of usd convenience functions

## [0.5.13] - 2023-06-07
### Fixed
-  Crash when name was not provided for a material, added check for null pointer and unit test

## [0.5.12] - 2023-05-16
### Fixed
-  Removed duplicated code to copy collision geometry from the mesh visuals.

## [0.5.11] - 2023-05-06
### Added
-  Collision geometry from visual meshes.

## [0.5.10] - 2023-05-04
### Added
- Code overview and limitations in README.

## [0.5.9] - 2023-02-28
### Fixed
- Appearance of carb warnings when setting the joint damping and stiffness to 0 for `NONE` drive type

## [0.5.8] - 2023-02-22
### Fixed
- removed max joint effort scaling by 60 during import
- removed custom collision api when the shape is a cylinder

## [0.5.7] - 2023-02-17
### Added
- Unit test for joint limits.
- URDF data file for joint limit unit test (test_limits.urdf)

## [0.5.6] - 2023-02-14
### Fixed
- Imported negative URDF effort and velocity joint constraints set the physics constraint value to infinity.

## [0.5.5] - 2023-01-06
### Fixed
- onclick_fn warning when creating UI

## [0.5.4] - 2022-10-13
### Fixed
- Fixes materials on instanceable imports

## [0.5.3] - 2022-10-13
### Fixed
- Added Test for import stl with custom material

## [0.5.2] - 2022-09-07
### Fixed
- Fixes for kit 103.5

## [0.5.1] - 2022-01-02

### Changed
- Use omni.kit.viewport.utility when setting camera

## [0.5.0] - 2022-08-30

### Changed
- Remove direct legacy viewport calls

## [0.4.1] - 2022-08-30

### Changed
- Modified default gains in URDF -> USD converter to match gains for Franka and UR10 robots

## [0.4.0] - 2022-08-09

### Added
- Cobotta 900 urdf data files

## [0.3.1] - 2022-08-08

### Fixed
- Missing argument in example docstring

## [0.3.0] - 2022-07-09

### Added
- Add instanceable option to importer

## [0.2.2] - 2022-06-02

### Changed
- Fix title for file picker

## [0.2.1] - 2022-05-23

### Changed
- Fix units for samples

## [0.2.0] - 2022-05-17

### Changed
- Add joint values API

## [0.1.16] - 2022-04-19

### Changed
- Add Texture import compatibility for Windows.

## [0.1.16] - 2022-02-08

### Changed
- Revamped UI

## [0.1.15] - 2021-12-20

### Changed
- Fixed bug where missing mesh on part with urdf material assigned would crash on material binding in a non-existing prim.

## [0.1.14] - 2021-12-20

### Changed
- Fix bug where material was indexed by name and removing false duplicates.
- Add Normal subdivision group import parameter.

## [0.1.13] - 2021-12-10

### Changed
- Texture support for OBJ and Collada assets.
- Remove bug where an invalid link on a joint would stop importing the remainder of the urdf. raises an error message instead.

## [0.1.12] - 2021-12-03

### Changed
- Default to save Imported assets on a new USD and reference it on open stage.
- Change base robot prim to also use orientOP instead of rotateOP
- Change behavior where any stage event (e.g selection changed) was resetting some options on the UI

## [0.1.11] - 2021-11-29

### Changed
- Use double precision for xform ops to match isaac sim defaults

## [0.1.10] - 2021-11-04

### Changed
- create physics scene is false for import config
- create physics scene will not create a scene if one exists
- set default prim is false for import config

## [0.1.9] - 2021-10-25

### Added
- Support to specify usd paths for urdf meshes.

### Changed
- distance_scale sets the stage to the same units for consistency
- None drive mode still applies DriveAPI, but keeps the stiffness/damping at zero
- rootJoint prim is renamed to root_joint for consistency with other joint names.

### Fixed
- warnings when setting attributes as double when they should have been float

## [0.1.8] - 2021-10-18

### Added
- Floating joints are ignored but place any child links at the correct location.

### Fixed
- Crash when urdf contained a floating joint

## [0.1.7] - 2021-09-23

### Added
- Default position drive damping to UI

### Fixed
- Default config parameters are now respected

## [0.1.6] - 2021-08-31

### Changed
- Updated to New UI
- Spheres and Cubes are treated as shapes
- Cylinders are by default imported with custom geometry enabled
- Joint drives are default force instead of acceleration

### Fixed
- Meshes were not imported correctly, fixed subdivision scheme setting

### Removed
- Parsing URDF is not a separate step with its own UI

## [0.1.5] - 2021-07-30

### Fixed
- Zero joint velocity issue
- Artifact when dragging URDF file due to transform matrix

## [0.1.4] - 2021-06-09

### Added
- Fixed bugs with default density

## [0.1.3] - 2021-05-26

### Added
- Fixed bugs with import units
- Streamlined UI and fixed missing elements
- Fixed issues with creating new stage on import

## [0.1.2] - 2020-12-11

### Added
- Unit tests to extension
- Add test urdf files
- Fix unit issues with samples
- Fix unit conversion issue with import

## [0.1.1] - 2020-12-03

### Added
- Sample URDF files for carter, franka ur10 and kaya

## [0.1.0] - 2020-12-03

### Added
- Initial version of URDF importer extension
