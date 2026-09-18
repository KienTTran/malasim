## 🔖 Versioning Information

This project uses an auto-generated file, `src/apps/malasim/version_generated.h`, to embed version details into the build. This file includes:

- The current version number (e.g., `6.0.1`)
- The branch name in the format `username/repo/branch`
- The short commit SHA

### CI Behavior

During each CI build:

1. The workflow reads `src/apps/malasim/version.h`, which contains placeholders for branch and commit information.
2. It generates `version_generated.h` by replacing these placeholders with actual values.
3. If the build is on a trusted branch or internal pull request, the CI commits and pushes `version_generated.h` back to the repository.

### Local Development

When building locally:

- Ensure you have the latest `version_generated.h` by pulling the latest changes from the repository.
- The build system includes `version_generated.h` if it exists; otherwise, it falls back to `version.h`.

This approach ensures consistent version information across builds and simplifies version tracking.
