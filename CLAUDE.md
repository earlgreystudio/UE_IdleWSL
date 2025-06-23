# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Unreal Engine project repository named "UE_IdleWSL" that is currently in its initial setup phase. The repository contains basic configuration files but no Unreal Engine project files (.uproject) or source code yet.

## Repository Structure

- Currently minimal with only README.md, VS Code workspace, and .gitignore
- Uses standard Unreal Engine .gitignore patterns for C++/Blueprint projects
- Configured for WSL development environment

## Development Setup

This project appears to be set up for Unreal Engine development in WSL (Windows Subsystem for Linux). When the actual Unreal project is created:

### Common Commands
- **Create Project**: Use Unreal Engine Editor or `UnrealBuildTool` commands
- **Build**: `UnrealBuildTool <ProjectName> <Platform> <Configuration>`
- **Package**: Use Unreal Editor's packaging tools or automation commands

### File Structure (when project is created)
- `Source/` - C++ source code
- `Content/` - Blueprint assets, materials, meshes, etc.
- `Config/` - Project configuration files
- `Plugins/` - Project-specific plugins
- `<ProjectName>.uproject` - Main project file

## Notes

The repository is currently empty of Unreal Engine files. When adding the actual Unreal project:
- Ensure .uproject file is in the root directory
- C++ code should follow Unreal's naming conventions (AMyActor, UMyComponent, etc.)
- Blueprint assets go in Content/ folder with organized subfolder structure