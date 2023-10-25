import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.freeDiskSpace
import jetbrains.buildServer.configs.kotlin.v2019_2.buildFeatures.commitStatusPublisher
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script
import jetbrains.buildServer.configs.kotlin.v2019_2.failureConditions.BuildFailureOnMetric
import jetbrains.buildServer.configs.kotlin.v2019_2.failureConditions.failOnMetricChange
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.finishBuildTrigger
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.schedule
import jetbrains.buildServer.configs.kotlin.v2019_2.triggers.vcs
import jetbrains.buildServer.configs.kotlin.v2019_2.vcs.GitVcsRoot

/*
The settings script is an entry point for defining a TeamCity
project hierarchy. The script should contain a single call to the
project() function with a Project instance or an init function as
an argument.

VcsRoots, BuildTypes, Templates, and subprojects can be
registered inside the project using the vcsRoot(), buildType(),
template(), and subProject() methods respectively.

To debug settings scripts in command-line, run the

    mvnDebug org.jetbrains.teamcity:teamcity-configs-maven-plugin:generate

command and attach your debugger to the port 8000.

To debug in IntelliJ Idea, open the 'Maven Projects' tool window (View
-> Tool Windows -> Maven Projects), find the generate task node
(Plugins -> teamcity-configs -> teamcity-configs:generate), the
'Debug' option is available in the context menu for the task.
*/

version = "2021.1"

project {
    description = "Kit App Template"

    params {
        param("env.OV_BRANCH_NAME", "no need")
    }

    subProject(Master)
}

object Master : Project({
    name = "master"

    buildType(Master_AutoupdateKitSdk)
    buildType(Master_ReleaseNewVersion)
    buildType(Master_BuildAndValidation)

    params {
        param("omni.branchname", "master")
    }
    subProjectsOrder = arrayListOf(RelativeId("Master_Building"), RelativeId("Master_Testing"), RelativeId("Master_Publishing"))

    subProject(Master_Testing)
    subProject(Master_Building)
    subProject(Master_Publishing)
})

object Master_AutoupdateKitSdk : BuildType({
    name = "update kit sdk and extensions"
    description = "Looks for newer kit-sdk and extension versions, pushes changes back"

    params {
        param("omni.branchname", "daily")
        param("env.BUILD_NUMBER", "%build.number%")
    }

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = """call tools\ci\autoupdate-kit-sdk\step.bat"""
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    triggers {
        schedule {
            enabled = false // Uncomment to run daily

            schedulingPolicy = daily {
                hour = 6
                minute = 30
            }
            branchFilter = """
                +:daily
            """.trimIndent()
            triggerBuild = always()
            withPendingChangesOnly = false
        }
    }

    failureConditions {
        executionTimeoutMin = 10
    }

    requirements {
        doesNotExist("system.feature.nvidia.gpu.count")
        contains("teamcity.agent.jvm.os.name", "Windows 10")
    }
})

object Master_ReleaseNewVersion : BuildType({
    name = "release a new version"
    description = "Increments version, generates changelog, pushes changes back"

    params {
        param("omni.branchname", "daily")
        param("env.BUILD_NUMBER", "%build.number%")
    }

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = """call tools\ci\release-new-version\step.bat"""
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    triggers {
        vcs {
            triggerRules = "+:deps/kit-sdk.packman.xml"
            branchFilter = "+:daily"
        }
    }

    failureConditions {
        executionTimeoutMin = 10
    }

    dependencies {
        snapshot(Master_Building_BuildLinuxX8664) {
            onDependencyFailure = FailureAction.FAIL_TO_START
            onDependencyCancel = FailureAction.FAIL_TO_START
        }
        snapshot(Master_Building_BuildWindowsX8664) {
            onDependencyFailure = FailureAction.FAIL_TO_START
            onDependencyCancel = FailureAction.FAIL_TO_START
        }
        snapshot(Master_Testing_TestLinuxX8664) {
            onDependencyFailure = FailureAction.ADD_PROBLEM
            onDependencyCancel = FailureAction.ADD_PROBLEM
        }
        snapshot(Master_Testing_TestWindowsX8664) {
            onDependencyFailure = FailureAction.ADD_PROBLEM
            onDependencyCancel = FailureAction.ADD_PROBLEM
        }
    }

    requirements {
        doesNotExist("system.feature.nvidia.gpu.count")
        contains("teamcity.agent.jvm.os.name", "Windows 10")
    }
})

object Master_BuildAndValidation : BuildType({
    name = "build validation (build, test, publish)"

    type = BuildTypeSettings.Type.COMPOSITE

    vcs {
        root(RelativeId("GitlabMaster"))

        showDependenciesChanges = true
    }

    triggers {
        vcs {
            branchFilter = """
                +:<default>
                +:merge-requests*
                +:release/*
            """.trimIndent()
        }
    }

    dependencies {
        snapshot(Master_Building_BuildWindowsX8664) {
            onDependencyFailure = FailureAction.FAIL_TO_START
        }
        snapshot(Master_Building_BuildLinuxX8664) {
            onDependencyFailure = FailureAction.FAIL_TO_START
        }
        snapshot(Master_Testing_TestLinuxX8664) {
        }
        snapshot(Master_Testing_TestWindowsX8664) {
        }
        //snapshot(Master_Publishing_PublishExtensions) {
        //}
    }
})

object Master_Building : Project({
    name = "building"

    buildType(Master_Building_BuildLinuxX8664)
    buildType(Master_Building_BuildLinuxAarch64)
    buildType(Master_Building_GenerateBuildNumber)
    buildType(Master_Building_BuildWindowsX8664)
})

object Master_Building_BuildLinuxAarch64 : BuildType({
    name = "build (linux-aarch64)"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    params {
        param("env.BUILD_NUMBER", "%build.number%")
    }

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = "./tools/ci/building/build-linux-x86_64/step.sh"
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    failureConditions {
        executionTimeoutMin = 30
    }

    dependencies {
        snapshot(Master_Building_GenerateBuildNumber) {
            onDependencyFailure = FailureAction.FAIL_TO_START
            onDependencyCancel = FailureAction.FAIL_TO_START
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Linux")
        doesNotExist("system.feature.nvidia.gpu.name")
    }
})

object Master_Building_BuildLinuxX8664 : BuildType({
    name = "build (linux-x86_64)"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    params {
        param("env.BUILD_NUMBER", "%build.number%")
    }

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = "./tools/ci/building/build-linux-x86_64/step.sh"
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    failureConditions {
        executionTimeoutMin = 30
    }

    features {
        commitStatusPublisher {
            publisher = gitlab {
                gitlabApiUrl = "https://gitlab-master.nvidia.com/api/v4"
                accessToken = "%env.GITLAB_AUTH_TOKEN%"
            }
        }
    }

    dependencies {
        snapshot(Master_Building_GenerateBuildNumber) {
            onDependencyFailure = FailureAction.FAIL_TO_START
            onDependencyCancel = FailureAction.FAIL_TO_START
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Linux")
        doesNotExist("system.feature.nvidia.gpu.name")
    }
})

object Master_Building_BuildWindowsX8664 : BuildType({
    name = "build (windows-x86_64)"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    params {
        param("env.BUILD_NUMBER", "%build.number%")
    }

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = "tools/ci/building/build-windows-x86_64/step.bat"
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    failureConditions {
        executionTimeoutMin = 30
    }

    features {
        commitStatusPublisher {
            publisher = gitlab {
                gitlabApiUrl = "https://gitlab-master.nvidia.com/api/v4"
                accessToken = "%env.GITLAB_AUTH_TOKEN%"
            }
        }
    }

    dependencies {
        snapshot(Master_Building_GenerateBuildNumber) {
            onDependencyFailure = FailureAction.FAIL_TO_START
            onDependencyCancel = FailureAction.FAIL_TO_START
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Windows")
        doesNotExist("system.feature.nvidia.gpu.name")
    }
})

object Master_Building_GenerateBuildNumber : BuildType({
    name = "generate build number"

    params {
        param("env.OV_BRANCH_NAME", "master")
    }

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = """call tools\ci\building\generate-build-number\step.bat"""
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Windows")
        doesNotExist("system.feature.nvidia.gpu.count")
    }
})


object Master_Publishing : Project({
    name = "publishing"

    buildType(Master_Publishing_PublishExtensions)
    buildType(Master_Publishing_PublishDocs)
    buildType(Master_Publishing_SignWindowsBinaries)
    buildType(Master_Publishing_DeployLauncher)
})

object Master_Publishing_PublishExtensions : BuildType({
    name = "publish extensions (all platforms)"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    vcs {
        root(RelativeId("GitlabMaster"))
        cleanCheckout = true
        showDependenciesChanges = true
    }

    steps {
        script {
            conditions {
                doesNotContain("teamcity.build.branch", "merge-requests/")
            }

            scriptContent = "tools/ci/publishing/publish-extensions/step.sh"
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    features {
        freeDiskSpace {
            failBuild = false
        }
    }

    features {
        commitStatusPublisher {
            publisher = gitlab {
                gitlabApiUrl = "https://gitlab-master.nvidia.com/api/v4"
                accessToken = "%env.GITLAB_AUTH_TOKEN%"
            }
        }
    }

    dependencies {
        dependency(Master_Building_BuildLinuxX8664) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = """
                    *.zip => _build/packages
                """.trimIndent()
            }
        }
        dependency(Master_Building_BuildWindowsX8664) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = """
                    *.zip => _build/packages
                """.trimIndent()
            }
        }
        snapshot(Master_Testing_TestWindowsX8664) {
            onDependencyFailure = FailureAction.FAIL_TO_START
            onDependencyCancel = FailureAction.FAIL_TO_START
        }
        snapshot(Master_Testing_TestLinuxX8664) {
            onDependencyFailure = FailureAction.FAIL_TO_START
            onDependencyCancel = FailureAction.FAIL_TO_START
        }
    }

    requirements {
        doesNotExist("system.feature.nvidia.gpu.driver.major")
        contains("teamcity.agent.jvm.os.name", "Linux")
    }
})

object Master_Publishing_PublishDocs : BuildType({
    name = "publish docs"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
        showDependenciesChanges = true
    }

    triggers {
        finishBuildTrigger {
            buildType = "${Master_BuildAndValidation.id}"
            successfulOnly = true
        }
    }

    steps {
        script {
            scriptContent = """tools\ci\publishing\publish-docs\step.bat"""
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    features {
        freeDiskSpace {
            failBuild = false
        }
    }

    dependencies {
        dependency(Master_Building_BuildWindowsX8664) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                artifactRules = """
                    docs*.7z!** => .
                """.trimIndent()
            }
        }
    }

    requirements {
        doesNotExist("system.feature.nvidia.gpu.driver.major")
        exists("system.feature.windows.version")
    }

    params {
        param("env.AWS_ACCESS_KEY_ID", "%omniverse-docs.AWS_ACCESS_KEY_ID%")
        param("env.AWS_SECRET_ACCESS_KEY", "%omniverse-docs.AWS_SECRET_ACCESS_KEY%")
    }
})

object Master_Testing : Project({
    name = "testing"

    buildType(Master_Testing_TestLinuxX8664)
    buildType(Master_Testing_TestWindowsX8664)
})

object Master_Testing_TestLinuxX8664 : BuildType({
    name = "test (linux-x86_64)"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = "./tools/ci/testing/test-linux-x86_64/step.sh"
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    features {
        commitStatusPublisher {
            publisher = gitlab {
                gitlabApiUrl = "https://gitlab-master.nvidia.com/api/v4"
                accessToken = "%env.GITLAB_AUTH_TOKEN%"
            }
        }
    }

    dependencies {
        dependency(Master_Building_BuildLinuxX8664) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                cleanDestination = true
                artifactRules = "*.zip => _build/packages"
            }
        }
    }

    requirements {
        exists("system.feature.nvidia.gpu.driver.major")
        contains("teamcity.agent.jvm.os.name", "Linux")
    }
})

object Master_Testing_TestWindowsX8664 : BuildType({
    name = "test (windows-x86_64)"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = """call tools\ci\testing\test-windows-x86_64\step.bat"""
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    features {
        commitStatusPublisher {
            publisher = gitlab {
                gitlabApiUrl = "https://gitlab-master.nvidia.com/api/v4"
                accessToken = "%env.GITLAB_AUTH_TOKEN%"
            }
        }
    }

    dependencies {
        dependency(Master_Building_BuildWindowsX8664) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                cleanDestination = true
                artifactRules = "*.zip => _build/packages"
            }
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Windows")
        exists("system.feature.nvidia.gpu.driver.major")
    }
})

object Master_Publishing_SignWindowsBinaries : BuildType({
    name = "sign windows binaries"

    buildNumberPattern = "${Master_Building_GenerateBuildNumber.depParamRefs.buildNumber}"

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = """call tools\ci\publishing\sign-windows-binaries\step.bat"""
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    triggers {
        vcs {
            enabled = false
        }
    }

    failureConditions {
        executionTimeoutMin = 60
        failOnMetricChange {
            metric = BuildFailureOnMetric.MetricType.ARTIFACT_SIZE
            units = BuildFailureOnMetric.MetricUnit.DEFAULT_UNIT
            comparison = BuildFailureOnMetric.MetricComparison.LESS
            compareTo = value()
            param("metricThreshold", "1KB")
            param("anchorBuild", "lastSuccessful")
        }
    }

    dependencies {
        dependency(Master_Building_BuildWindowsX8664) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                cleanDestination = true
                artifactRules = "*.release.zip => _unsignedpackages"
            }
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Windows 10")
        contains("teamcity.agent.name", "signer")
    }
})


object Master_Publishing_DeployLauncher : BuildType({
    name = "publish to the Launcher"
    description = "Publish launcher packman packages and push configs to the Launcher pipeline repo"

    vcs {
        root(RelativeId("GitlabMaster"))

        cleanCheckout = true
    }

    steps {
        script {
            scriptContent = """call tools\ci\publishing\publish-launcher-packages\step.bat"""
            param("org.jfrog.artifactory.selectedDeployableServer.downloadSpecSource", "Job configuration")
            param("org.jfrog.artifactory.selectedDeployableServer.useSpecs", "false")
            param("org.jfrog.artifactory.selectedDeployableServer.uploadSpecSource", "Job configuration")
        }
    }

    triggers {
        vcs {
            enabled = true
            triggerRules = "+:VERSION.md"
            branchFilter = """
                +:main
                +:master
                +:release/*
                +:daily
            """.trimIndent()
        }
    }

    dependencies {
        dependency(Master_Publishing_SignWindowsBinaries) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                cleanDestination = true
                artifactRules = """
                    *.signed.zip => _build/packages
                """.trimIndent()
            }
        }
        dependency(Master_Building_BuildLinuxX8664) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
                onDependencyCancel = FailureAction.FAIL_TO_START
            }

            artifacts {
                cleanDestination = true
                artifactRules = "* => _build/packages"
            }
        }
    }

    requirements {
        contains("teamcity.agent.jvm.os.name", "Windows")
        doesNotExist("system.feature.nvidia.gpu.count")
        doesNotContain("system.agent.name", "signer")
    }

    disableSettings("RQ_2107")
})
