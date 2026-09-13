/* SPDX-License-Identifier: MIT */
#ifndef RINERROR_ERROR_H
#define RINERROR_ERROR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RIN_ERROR_VERSION_1 1u

typedef enum RinErrorDomain {
    RIN_ERROR_DOMAIN_ERRNO = 1u,
    RIN_ERROR_DOMAIN_NEGATIVE_SYSCALL = 2u,
    RIN_ERROR_DOMAIN_RIN_SDK = 3u,
    RIN_ERROR_DOMAIN_RIN_RESULT = 4u,
    RIN_ERROR_DOMAIN_IPC = 5u,
    RIN_ERROR_DOMAIN_ENUM_STATUS = 6u,
    RIN_ERROR_DOMAIN_WIN32 = 7u
} RinErrorDomain;

typedef enum RinErrorKind {
    RIN_ERROR_SUCCESS = 0u,
    RIN_ERROR_INVALID_ARGUMENT = 1u,
    RIN_ERROR_NOT_FOUND = 2u,
    RIN_ERROR_PERMISSION_DENIED = 3u,
    RIN_ERROR_RESOURCE_EXHAUSTED = 4u,
    RIN_ERROR_ALREADY_EXISTS = 5u,
    RIN_ERROR_BUSY = 6u,
    RIN_ERROR_TIMEOUT = 7u,
    RIN_ERROR_CANCELLED = 8u,
    RIN_ERROR_IO = 9u,
    RIN_ERROR_WOULD_BLOCK = 10u,
    RIN_ERROR_MALFORMED = 11u,
    RIN_ERROR_STALE = 12u,
    RIN_ERROR_REVOKED = 13u,
    RIN_ERROR_UNSUPPORTED = 14u,
    RIN_ERROR_INTEGRITY = 15u,
    RIN_ERROR_DEVICE_LOST = 16u,
    RIN_ERROR_INTERRUPTED = 17u,
    RIN_ERROR_UNKNOWN = 18u
} RinErrorKind;

enum {
    RIN_ERROR_FLAG_MALFORMED = 0x00000001u,
    RIN_ERROR_FLAG_REVOKED = 0x00000002u,
    RIN_ERROR_FLAG_CANCELLED = 0x00000004u,
    RIN_ERROR_FLAG_RETRYABLE = 0x00000008u
};

typedef enum RinErrorStatus {
    RIN_ERROR_STATUS_OK = 0,
    RIN_ERROR_STATUS_INVALID_ARGUMENT = -1,
    RIN_ERROR_STATUS_BAD_VERSION = -2,
    RIN_ERROR_STATUS_BAD_FLAGS = -3,
    RIN_ERROR_STATUS_BAD_DOMAIN = -4,
    RIN_ERROR_STATUS_BAD_KIND = -5
} RinErrorStatus;

typedef struct RinErrorV1 {
    uint32_t struct_size;
    uint16_t version;
    uint16_t domain;
    int64_t source_code;
    uint32_t kind;
    uint32_t flags;
    int32_t canonical_errno;
    uint32_t reserved;
} RinErrorV1;

int rin_error_validate(const RinErrorV1* error);
int rin_error_from_code(uint16_t domain, int64_t code, RinErrorV1* output);
int rin_error_from_status(uint16_t domain, int64_t code, uint32_t flags,
                          RinErrorV1* output);
int rin_error_from_errno(int error_number, RinErrorV1* output);
int rin_error_from_negative_syscall(intptr_t result, RinErrorV1* output);
int rin_error_from_win32(uint32_t error_code, RinErrorV1* output);

int rin_error_is_success(const RinErrorV1* error);
int rin_error_is_retryable(const RinErrorV1* error);
int rin_error_is_cancellation(const RinErrorV1* error);
int rin_error_is_malformed(const RinErrorV1* error);
int rin_error_is_revoked(const RinErrorV1* error);
int rin_error_to_errno(const RinErrorV1* error);
const char* rin_error_kind_name(uint32_t kind);

#ifdef __cplusplus
}
#endif

#endif /* RINERROR_ERROR_H */
