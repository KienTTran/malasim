/*
 * version.h
 *
 * This file exposes the simulation version so that it can be retrieved by other
 * scripts.
 */
#pragma once

#ifndef GIT_REPO_INFO
#define GIT_REPO_INFO "unknown/unknown"
#endif

#ifndef GIT_BRANCH_NAME
#define GIT_BRANCH_NAME "unknown"
#endif

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "unknown"
#endif

inline constexpr const char* VERSION =
    "6.1.0 (branch: " GIT_REPO_INFO "/" GIT_BRANCH_NAME ", commit: " GIT_COMMIT_HASH ")";

