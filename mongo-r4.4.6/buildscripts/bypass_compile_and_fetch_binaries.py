#!/usr/bin/env python3
"""Bypass compile and fetch binaries."""
from collections import namedtuple
import json
import logging
import os
import sys
import tarfile
from tempfile import TemporaryDirectory
import urllib.error
import urllib.parse
from urllib.parse import urlparse
import urllib.request
from typing import Any, Dict, List

import click

from evergreen.api import RetryingEvergreenApi, EvergreenApi, Build, Task
from git.repo import Repo
import requests
import structlog
from structlog.stdlib import LoggerFactory
import yaml

# Get relative imports to work when the package is not installed on the PYTHONPATH.
if __name__ == "__main__" and __package__ is None:
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# pylint: disable=wrong-import-position
from buildscripts.ciconfig.evergreen import parse_evergreen_file
# pylint: enable=wrong-import-position

structlog.configure(logger_factory=LoggerFactory())
LOGGER = structlog.get_logger(__name__)

EVG_CONFIG_FILE = ".evergreen.yml"
DESTDIR = os.getenv("DESTDIR")

_IS_WINDOWS = (sys.platform == "win32" or sys.platform == "cygwin")

# If changes are only from files in the bypass_files list or the bypass_directories list, then
# bypass compile, unless they are also found in the BYPASS_EXTRA_CHECKS_REQUIRED lists. All other
# file changes lead to compile.
BYPASS_WHITELIST = {
    "files": {
        "etc/evergreen.yml",
    },
    "directories": {
        "buildscripts/",
        "jstests/",
        "pytests/",
    },
}  # yapf: disable

# These files are exceptions to any whitelisted directories in bypass_directories. Changes to
# any of these files will disable compile bypass. Add files you know should specifically cause
# compilation.
BYPASS_BLACKLIST = {
    "files": {
        "buildscripts/errorcodes.py",
        "buildscripts/make_archive.py",
        "buildscripts/moduleconfig.py",
        "buildscripts/msitrim.py",
        "buildscripts/packager_enterprise.py",
        "buildscripts/packager.py",
        "buildscripts/scons.py",
        "buildscripts/utils.py",
    },
    "directories": {
        "buildscripts/idl/",
        "src/",
    }
}  # yapf: disable

# Changes to the BYPASS_EXTRA_CHECKS_REQUIRED_LIST may or may not allow bypass compile, depending
# on the change. If a file is added to this list, the _check_file_for_bypass() function should be
# updated to perform any extra checks on that file.
BYPASS_EXTRA_CHECKS_REQUIRED = {
    "etc/evergreen.yml",
}  # yapf: disable

# Expansions in etc/evergreen.yml that must not be changed in order to bypass compile.
EXPANSIONS_TO_CHECK = {
    "compile_flags",
}  # yapf: disable

# SERVER-21492 related issue where without running scons the jstests/libs/key1
# and key2 files are not chmod to 0600. Need to change permissions since we bypass SCons.
ARTIFACTS_NEEDING_PERMISSIONS = {
    os.path.join("jstests", "libs", "key1"): 0o600,
    os.path.join("jstests", "libs", "key2"): 0o600,
    os.path.join("jstests", "libs", "keyForRollover"): 0o600,
}

ARTIFACT_ENTRIES_MAP = {
    "mongo_binaries": "Binaries",
}

ARTIFACTS_TO_EXTRACT = {
    "mongobridge",
    "mongotmock",
    "wt",
}

TargetBuild = namedtuple("TargetBuild", [
    "project",
    "revision",
    "build_variant",
])


def executable_name(pathname: str) -> str:
    """Return the executable name."""
    # Ensure that executable files on Windows have a ".exe" extension.
    if _IS_WINDOWS and os.path.splitext(pathname)[1] != ".exe":
        pathname = f"{pathname}.exe"

    if DESTDIR:
        return os.path.join(DESTDIR, "bin", pathname)

    return pathname


def archive_name(archive: str) -> str:
    """Return the archive name."""
    # Ensure the right archive extension is used for Windows.
    if _IS_WINDOWS:
        return f"{archive}.zip"
    return f"{archive}.tgz"


def write_out_bypass_compile_expansions(patch_file, **expansions):
    """Write out the macro expansions to given file."""
    with open(patch_file, "w") as out_file:
        LOGGER.info("Saving compile bypass expansions", patch_file=patch_file,
                    expansions=expansions)
        yaml.safe_dump(expansions, out_file, default_flow_style=False)


def write_out_artifacts(json_file, artifacts):
    """Write out the JSON file with URLs of artifacts to given file."""
    with open(json_file, "w") as out_file:
        LOGGER.info("Generating artifacts.json from pre-existing artifacts", json=json.dumps(
            artifacts, indent=4))
        json.dump(artifacts, out_file)


def _create_bypass_path(prefix, build_id, name):
    """
    Create the path for the bypass expansions.

    :param prefix: Prefix of the path.
    :param build_id: Build-Id to use.
    :param name: Name of file.
    :return: Path to use for bypass expansion.
    """
    return archive_name(f"{prefix}/{name}-{build_id}")


def _artifact_to_bypass_path(project: str, artifact_url: str) -> str:
    """
    Get the unique part of the path for the given artifact url.

    :param project: Evergreen project being run in.
    :param artifact_url: Full url or artifact.
    :return: Unique part of URL containing path to artifact.
    """
    start_idx = artifact_url.find(project)
    return artifact_url[start_idx:]


def generate_bypass_expansions(target: TargetBuild, artifacts_list: List) -> Dict[str, Any]:
    """
    Create a dictionary of the generated bypass expansions.

    :param target: Build being targeted.
    :param artifacts_list: List of artifacts being bypassed.
    :returns: Dictionary of expansions to update.
    """
    # Convert the artifacts list to a dictionary for easy lookup.
    artifacts_dict = {artifact["name"].strip(): artifact["link"] for artifact in artifacts_list}
    bypass_expansions = {
        key: _artifact_to_bypass_path(target.project, artifacts_dict[value])
        for key, value in ARTIFACT_ENTRIES_MAP.items()
    }
    bypass_expansions["bypass_compile"] = True
    return bypass_expansions


def _get_original_etc_evergreen(path):
    """
    Get the etc/evergreen configuration before the changes were made.

    :param path: path to etc/evergreen.
    :return: An EvergreenProjectConfig for the previous etc/evergreen file.
    """
    repo = Repo(".")
    previous_contents = repo.git.show([f"HEAD:{path}"])
    with TemporaryDirectory() as tmpdir:
        file_path = os.path.join(tmpdir, "evergreen.yml")
        with open(file_path, "w") as fp:
            fp.write(previous_contents)
        return parse_evergreen_file(file_path)


def _check_etc_evergreen_for_bypass(path, build_variant):
    """
    Check if changes to etc/evergreen can be allowed to bypass compile.

    :param path: Path to etc/evergreen file.
    :param build_variant: Build variant to check.
    :return: True if changes can bypass compile.
    """
    variant_before = _get_original_etc_evergreen(path).get_variant(build_variant)
    variant_after = parse_evergreen_file(path).get_variant(build_variant)

    for expansion in EXPANSIONS_TO_CHECK:
        if variant_before.expansion(expansion) != variant_after.expansion(expansion):
            return False

    return True


def _check_file_for_bypass(file, build_variant):
    """
    Check if changes to the given file can be allowed to bypass compile.

    :param file: File to check.
    :param build_variant: Build Variant to check.
    :return: True if changes can bypass compile.
    """
    if file == "etc/evergreen.yml":
        return _check_etc_evergreen_for_bypass(file, build_variant)

    return True


def _file_in_group(filename, group):
    """
    Determine if changes to the given filename require compile to be run.

    :param filename: Filename to check.
    :param group: Dictionary containing files and filename to check.
    :return: True if compile should be run for filename.
    """
    if "files" not in group:
        raise TypeError("No list of files to check.")
    if filename in group["files"]:
        return True

    if "directories" not in group:
        raise TypeError("No list of directories to check.")
    if any(filename.startswith(directory) for directory in group["directories"]):
        return True

    return False


def should_bypass_compile(patch_file, build_variant):
    """
    Determine whether the compile stage should be bypassed based on the modified patch files.

    We use lists of files and directories to more precisely control which modified patch files will
    lead to compile bypass.
    :param patch_file: A list of all files modified in patch build.
    :param build_variant: Build variant where compile is running.
    :returns: True if compile should be bypassed.
    """
    with open(patch_file, "r") as pch:
        for filename in pch:
            filename = filename.rstrip()
            # Skip directories that show up in 'git diff HEAD --name-only'.
            if os.path.isdir(filename):
                continue

            log = LOGGER.bind(filename=filename)
            if _file_in_group(filename, BYPASS_BLACKLIST):
                log.warning("Compile bypass disabled due to blacklisted file")
                return False

            if not _file_in_group(filename, BYPASS_WHITELIST):
                log.warning("Compile bypass disabled due to non-whitelisted file")
                return False

            if filename in BYPASS_EXTRA_CHECKS_REQUIRED:
                if not _check_file_for_bypass(filename, build_variant):
                    log.warning("Compile bypass disabled due to extra checks for file.")
                    return False

    return True


def find_build_for_previous_compile_task(evg_api: EvergreenApi, target: TargetBuild) -> Build:
    """
    Find build_id of the base revision.

    :param evg_api: Evergreen.py object.
    :param target: Build being targeted.
    :return: build_id of the base revision.
    """
    project_prefix = target.project.replace("-", "_")
    version_of_base_revision = f"{project_prefix}_{target.revision}"
    version = evg_api.version_by_id(version_of_base_revision)
    build = version.build_by_variant(target.build_variant)
    return build


def find_previous_compile_task(build: Build) -> Task:
    """
    Find compile task that should be used for skip compile.

    :param build: Build containing the desired compile task.
    :return: Evergreen.py object containing data about the desired compile task.
    """
    tasks = [task for task in build.get_tasks() if task.display_name == "compile"]
    assert len(tasks) == 1
    return tasks[0]


def extract_filename_from_url(url: str) -> str:
    """
    Extract the name of a file from the download url.

    :param url: Download URL to extract from.
    :return: filename part of the given url.
    """
    parsed_url = urlparse(url)
    filename = os.path.basename(parsed_url.path)
    return filename


def download_file(download_url: str, download_location: str) -> None:
    """
    Download the file at the specified path locally.

    :param download_url: URL to download.
    :param download_location: Path to store the downloaded file.
    """
    try:
        urllib.request.urlretrieve(download_url, download_location)
    except urllib.error.ContentTooShortError:
        LOGGER.warning(
            "The artifact could not be completely downloaded. Default"
            " compile bypass to false.", filename=download_location)
        raise ValueError("No artifacts were found for the current task")


def extract_artifacts(filename: str) -> None:
    """
    Extract interests contents from the artifacts tar file.

    :param filename: Path to artifacts file.
    """
    extract_files = {executable_name(artifact) for artifact in ARTIFACTS_TO_EXTRACT}
    with tarfile.open(filename, "r:gz") as tar:
        # The repo/ directory contains files needed by the package task. May
        # need to add other files that would otherwise be generated by SCons
        # if we did not bypass compile.
        subdir = [
            tarinfo for tarinfo in tar.getmembers()
            if tarinfo.name.startswith("repo/") or tarinfo.name in extract_files
        ]
        LOGGER.info("Extracting the files...", filename=filename,
                    files="\n".join(tarinfo.name for tarinfo in subdir))
        def is_within_directory(directory, target):
            
            abs_directory = os.path.abspath(directory)
            abs_target = os.path.abspath(target)
        
            prefix = os.path.commonprefix([abs_directory, abs_target])
            
            return prefix == abs_directory
        
        def safe_extract(tar, path=".", members=None, *, numeric_owner=False):
        
            for member in tar.getmembers():
                member_path = os.path.join(path, member.name)
                if not is_within_directory(path, member_path):
                    raise Exception("Attempted Path Traversal in Tar File")
        
            tar.extractall(path, members, numeric_owner=numeric_owner) 
            
        
        safe_extract(tar, members=subdir)


def rename_artifact(filename: str, target_name: str) -> None:
    """
    Rename the provided artifact file.

    :param filename: Path to artifact file to rename.
    :param target_name: New name to use.
    """
    extension = os.path.splitext(filename)[1]
    target_filename = f"{target_name}{extension}"

    LOGGER.info("Renaming", source=filename, target=target_filename)
    os.rename(filename, target_filename)


def validate_url(url: str) -> None:
    """
    Check the link exists, else raise an exception.

    :param url: Link to check.
    """
    requests.head(url).raise_for_status()


def fetch_artifacts(build: Build, revision: str) -> List[Dict[str, str]]:
    """
    Fetch artifacts from a given revision.

    :param build: Build id of the desired artifacts.
    :param revision: The revision being fetched from.
    :return: Artifacts from the revision.
    """
    LOGGER.info("Fetching artifacts", build_id=build.id, revision=revision)
    task = find_previous_compile_task(build)
    if task is None or not task.is_success():
        task_id = task.task_id if task else None
        LOGGER.warning(
            "Could not retrieve artifacts because the compile task for base commit"
            " was not available. Default compile bypass to false.", task_id=task_id)
        raise ValueError("No artifacts were found for the current task")

    LOGGER.info("Fetching pre-existing artifacts from compile task", task_id=task.task_id)
    artifacts = []
    for artifact in task.artifacts:
        filename = extract_filename_from_url(artifact.url)
        if filename.startswith(build.id):
            LOGGER.info("Retrieving artifacts.tgz", filename=filename)
            download_file(artifact.url, filename)
            extract_artifacts(filename)

        elif filename.startswith("debugsymbols"):
            LOGGER.info("Retrieving debug symbols", filename=filename)
            download_file(artifact.url, filename)
            rename_artifact(filename, "mongo-debugsymbols")

        elif filename.startswith("mongo-src"):
            LOGGER.info("Retrieving mongo source", filename=filename)
            download_file(artifact.url, filename)
            rename_artifact(filename, "distsrc")

        else:
            # For other artifacts we just add their URLs to the JSON file to upload.
            LOGGER.info("Linking base artifact to this patch build", filename=filename)
            validate_url(artifact.url)
            artifacts.append({
                "name": artifact.name,
                "link": artifact.url,
                "visibility": "private",
            })

    return artifacts


def update_artifact_permissions(permission_dict: Dict[str, int]) -> None:
    """
    Update the given files with the specified permissions.

    :param permission_dict: Keys of dict should be files to update, values should be permissions.
    """
    for path, perm in permission_dict.items():
        os.chmod(path, perm)


def gather_artifacts_and_update_expansions(build: Build, target: TargetBuild,
                                           json_artifact_file: str, expansions_file: str):
    """
    Fetch the artifacts for this build and save them to be used by other tasks.

    :param build: build containing artifacts.
    :param target: Target build being bypassed.
    :param json_artifact_file: File to write json artifacts to.
    :param expansions_file: File to write expansions to.
    """
    artifacts = fetch_artifacts(build, target.revision)
    update_artifact_permissions(ARTIFACTS_NEEDING_PERMISSIONS)
    write_out_artifacts(json_artifact_file, artifacts)

    LOGGER.info("Creating expansions files", target=target, build_id=build.id)

    expansions = generate_bypass_expansions(target, artifacts)
    write_out_bypass_compile_expansions(expansions_file, **expansions)


@click.command()
@click.option("--project", required=True, help="The evergreen project.")
@click.option("--build-variant", required=True,
              help="The build variant whose artifacts we want to use.")
@click.option("--revision", required=True, help="Base revision of the build.")
@click.option("--patch-file", required=True, help="A list of all files modified in patch build.")
@click.option("--out-file", required=True, help="File to write expansions to.")
@click.option("--json-artifact", required=True,
              help="The JSON file to write out the metadata of files to attach to task.")
def main(  # pylint: disable=too-many-arguments,too-many-locals,too-many-statements
        project: str, build_variant: str, revision: str, patch_file: str, out_file: str,
        json_artifact: str):
    """
    Create a file with expansions that can be used to bypass compile.

    If for any reason bypass compile is false, we do not write out the expansion. Only if we
    determine to bypass compile do we write out the expansions.
    \f

    :param project: The evergreen project.
    :param build_variant: The build variant whose artifacts we want to use.
    :param revision: Base revision of the build.
    :param patch_file: A list of all files modified in patch build.
    :param out_file: File to write expansions to.
    :param json_artifact: The JSON file to write out the metadata of files to attach to task.
    """
    logging.basicConfig(
        format="[%(asctime)s - %(name)s - %(levelname)s] %(message)s",
        level=logging.DEBUG,
        stream=sys.stdout,
    )

    target = TargetBuild(project=project, build_variant=build_variant, revision=revision)

    # Determine if we should bypass compile based on modified patch files.
    if should_bypass_compile(patch_file, build_variant):
        evg_api = RetryingEvergreenApi.get_api(config_file=EVG_CONFIG_FILE)
        build = find_build_for_previous_compile_task(evg_api, target)
        if not build:
            LOGGER.warning("Could not find build id. Default compile bypass to false.",
                           revision=revision, project=project)
            return

        gather_artifacts_and_update_expansions(build, target, json_artifact, out_file)


if __name__ == "__main__":
    main()  # pylint: disable=no-value-for-parameter
