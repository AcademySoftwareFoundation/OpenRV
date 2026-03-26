# General Code Review Standards

## Purpose

These instructions guide GitHub Copilot code review across all files in this repository.

## About Open RV

Open RV is an open-source, hardware-accelerated image and video sequence viewer built for visual effects (VFX) and animation production pipelines. It is maintained under the Academy Software Foundation (ASWF). It runs on macOS, Windows, and RHEL-based Linux distributions. The build targets the VFX Reference Platform specification for dependency compatibility across studios.

## Code Standards Reference

All code must follow the project's configuration files:

- **C++ formatting**: `.clang-format`
- **C++ linting**: `.clang-tidy`
- **Python formatting and linting**: `ruff.toml`
- **CMake formatting**: `cmake-format.json`
- **Pre-commit hooks**: `.pre-commit-config.yaml`

Review code against these standards and flag violations.

## Review Behavior

### Confidence Threshold

- **Only comment when you have 80% confidence** the issue is real and valid
- If uncertain about severity or whether it's actually a problem, don't comment
- If you notice a pattern that MIGHT be an issue but aren't certain, don't comment
- Do not speculate about hypothetical issues without clear evidence

### What to Review

- Security vulnerabilities
- Bugs and logic errors
- Breaking changes or API changes
- Performance issues
- Build system issues
- Missing error handling
- Changes made in the pull request
