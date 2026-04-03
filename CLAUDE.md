# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**ha-portainer-integration** is a C++ tool that talks to the Portainer API to fetch container/stack status and generates or updates a Home Assistant Lovelace dashboard view. The user regularly creates new stacks and containers in Portainer and needs an extensible way to keep the HA dashboard in sync.

- **License:** MIT
- **Language:** C++ (learning project — user is experienced in Python, C#, JS/TS but new to C++)
- **Remote:** https://github.com/lorainemg/ha-portainer-integration.git

## Development Approach

- **Incremental steps:** Implement features in small, well-explained increments. Explain what will be done before doing it, and summarize what was done after.
- **Teaching focus:** Use this project to introduce C++ concepts (memory management, RAII, templates, STL, smart pointers, etc.), drawing analogies to Python/C#/TS where helpful. Never use a C++ concept without fully explaining it first — do not assume the user already knows syntax from prior passing mentions.
- **Extensibility:** The dashboard generation should be easy to extend as new containers/stacks are added to Portainer.

## Current State

The repository contains only scaffolding files (README, LICENSE, .gitignore). No build commands, test commands, or linting are configured yet.
