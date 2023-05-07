# Copyright (C) 2022  Tiernan8r
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
import setuptools


# import requirements used in development so that the python
# project requirements match
with open('requirements.in', 'r') as fh:  # pylint: disable=unspecified-encoding # noqa: E501
    requirements = []
    for line in fh.readlines():
        line = line.strip()
        if line[0] != '#':
            requirements.append(line)

# import the README of the project
with open("README.md", "r") as fh:  # pylint: disable=unspecified-encoding
    long_description = fh.read()


setuptools.setup(
    name='receipt_post_processor',
    use_scm_version=True,  # tags the project version from the git tag
    setup_requires=['setuptools_scm', 'pytest-runner'],
    packages=setuptools.find_packages(where="src", exclude=['docs', 'tests']),
    package_dir={"": "src"},
    include_package_data=True,
    install_requires=requirements,
)
