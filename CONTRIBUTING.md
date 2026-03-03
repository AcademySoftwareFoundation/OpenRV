# Contributing

Thank you for your interest in contributing to Open RV. We are looking forward to collaborating with the community to evolve and improve Open RV.

## Project Roles and Responsibilities

Please read this document to learn how to contribute to Open RV.

Start by getting familiar with the [GOVERNANCE](GOVERNANCE.md) document. It details the rules and responsibilities for Contributors, Committers, and Technical Steering Committee members.

## Committers

The RV Open Source Project Committers are listed in alphabetical order, by first name, in [COMMITERS](COMMITERS.md)

## Contributor License Agreement

Before contributing code to Open RV source code, you must sign a Contributor License Agreement (CLA).

You will get the opportunity to sign the CLA when you create your first pull request: the Linux Foundation's EasyCLA system will guide you through the process of signing the CLA.

If you can't to use the EasyCLA system, you can always send a signed CLA to `ori-tsc@aswf.io` (making sure to include your *github username*), and wait for confirmation that we've received it.

Here are the two possible CLAs:

There are two contribution agreement types, one for individuals contributing independently, and one for corporations who want to manage a list of contributors for their organization. Please review the documents in the EasyCLA portal to determine which is the right one for you.

## Coding Conventions

Please follow the coding conventions and style in each file and in each library when adding new files.

## Git Workflow

### Pull Request Process

1. Fork the repository and create a feature branch from `main`.
2. Make your changes, following the coding conventions described above.
3. Open a pull request against `main`.
4. Ensure CI checks pass — in particular the **semantic PR title** check (see below).
5. Once reviewed and approved, a committer will merge your PR.

### Semantic PR Titles (Required)

All pull request titles **must** follow the [Conventional Commits](https://www.conventionalcommits.org/) format:

```
type: short description
```

A CI check will block merging if the title does not match this format. The allowed types are:

| Type    | When to use                                        | Appears in changelog |
| ------- | -------------------------------------------------- | -------------------- |
| `feat`  | A new feature                                      | Yes                  |
| `fix`   | A bug fix                                          | Yes                  |
| `perf`  | A performance improvement                          | Yes                  |
| `docs`  | Documentation-only changes                         | Yes                  |
| `build` | Changes to the build system or dependencies        | Yes                  |
| `ci`    | Changes to CI configuration or scripts             | Yes                  |
| `test`  | Adding or updating tests                           | No                   |
| `chore` | Maintenance tasks that don't affect published code | No                   |

For **breaking changes**, append `!` after the type (e.g., `feat!: remove legacy API`) or include a `BREAKING CHANGE:` footer in the PR body.

### Automated Releases

This project uses [git-cliff](https://git-cliff.org/) to automate changelog generation and version bumps. When PRs are merged to `main`, a GitHub Actions workflow runs git-cliff to parse commit messages (which reflect the squash-merged PR titles) and creates or updates a release pull request. Only PRs with types `feat`, `fix`, `perf`, `docs`, `build`, and `ci` appear in the generated changelog. Since PR titles are used as changelog entries, clear and descriptive titles are appreciated.

When the release PR is merged, a second workflow automatically creates a git tag and a GitHub Release with the generated release notes.
