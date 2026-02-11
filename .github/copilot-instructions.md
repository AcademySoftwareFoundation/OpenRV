# General Code Review Standards

## Purpose

These instructions guide Copilot code review across all files in this repository.

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

- **Only comment when you have 100% confidence** the issue is real and valid
- If uncertain about severity or whether it's actually a problem, don't comment
- If you notice a pattern that MIGHT be an issue but aren't certain, don't comment
- Do not speculate about hypothetical issues without clear evidence

### Pull request overview

- Maximum 1-2 sentences total
- Only mention if there are critical issues
- Example: "Found a buffer overflow on line 42 that needs fixing before merge." or "No issues found."

### What to Review

- Code that violates standards documented in `.clang-format`, `.clang-tidy`, `ruff.toml` or `cmake-format.json` configs
- Security vulnerabilities
- Bugs and logic errors
- Breaking changes or API changes
- Missing error handling
- Performance issues
- Build system issues
