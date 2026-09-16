/* SPDX-License-Identifier: MIT */
#define RINERROR_LEGACY_NAMES 1
#include "include/rinerror/error.h"

#include <errno.h>
#include <limits.h>

#if defined(EBADMSG)
#define RIN_ERRNO_BAD_MESSAGE EBADMSG
#else
#define RIN_ERRNO_BAD_MESSAGE EINVAL
#endif
#if defined(ECANCELED)
#define RIN_ERRNO_CANCELLED ECANCELED
#else
#define RIN_ERRNO_CANCELLED EINTR
#endif
#if defined(ESTALE)
#define RIN_ERRNO_STALE ESTALE
#else
#define RIN_ERRNO_STALE EINVAL
#endif
#if defined(ENOTSUP)
#define RIN_ERRNO_NOT_SUPPORTED ENOTSUP
#else
#define RIN_ERRNO_NOT_SUPPORTED EINVAL
#endif

static int domain_valid(uint16_t domain)
{
    return domain >= RIN_ERROR_DOMAIN_ERRNO &&
           domain <= RIN_ERROR_DOMAIN_WIN32;
}

static int flags_valid(uint32_t flags)
{
    const uint32_t semantic = flags & (RIN_ERROR_FLAG_MALFORMED |
                                        RIN_ERROR_FLAG_REVOKED |
                                        RIN_ERROR_FLAG_CANCELLED);
    return (flags & ~(RIN_ERROR_FLAG_MALFORMED |
                      RIN_ERROR_FLAG_REVOKED |
                      RIN_ERROR_FLAG_CANCELLED |
                      RIN_ERROR_FLAG_RETRYABLE)) == 0u &&
           (semantic == 0u || semantic == RIN_ERROR_FLAG_MALFORMED ||
            semantic == RIN_ERROR_FLAG_REVOKED ||
            semantic == RIN_ERROR_FLAG_CANCELLED);
}

int rin_error_validate(const RinErrorV1* error)
{
    if (error == NULL) return RIN_ERROR_STATUS_INVALID_ARGUMENT;
    if (error->struct_size != sizeof(*error) ||
        error->version != RIN_ERROR_VERSION_1 || error->reserved != 0u) {
        return RIN_ERROR_STATUS_BAD_VERSION;
    }
    if (!domain_valid(error->domain)) return RIN_ERROR_STATUS_BAD_DOMAIN;
    if (error->kind > RIN_ERROR_UNKNOWN) return RIN_ERROR_STATUS_BAD_KIND;
    if (!flags_valid(error->flags)) return RIN_ERROR_STATUS_BAD_FLAGS;
    if (error->kind == RIN_ERROR_SUCCESS && error->source_code != 0) {
        return RIN_ERROR_STATUS_BAD_KIND;
    }
    return RIN_ERROR_STATUS_OK;
}

static void initialize_error(uint16_t domain, int64_t code,
                             uint32_t kind, uint32_t flags,
                             RinErrorV1* output)
{
    output->struct_size = sizeof(*output);
    output->version = RIN_ERROR_VERSION_1;
    output->domain = domain;
    output->source_code = code;
    output->kind = kind;
    output->flags = flags;
    output->canonical_errno = 0;
    output->reserved = 0u;
}

static void clear_error(RinErrorV1* output)
{
    if (output != NULL) *output = (RinErrorV1){0};
}

static uint32_t kind_from_errno(int error_number)
{
    switch (error_number) {
    case 0: return RIN_ERROR_SUCCESS;
    case EINVAL: return RIN_ERROR_INVALID_ARGUMENT;
    case ENOENT: return RIN_ERROR_NOT_FOUND;
    case EACCES:
    case EPERM: return RIN_ERROR_PERMISSION_DENIED;
    case ENOMEM:
    case ENOSPC: return RIN_ERROR_RESOURCE_EXHAUSTED;
    case EEXIST: return RIN_ERROR_ALREADY_EXISTS;
    case EBUSY: return RIN_ERROR_BUSY;
    case ETIMEDOUT: return RIN_ERROR_TIMEOUT;
#if defined(ECANCELED) && ECANCELED != EINTR
    case ECANCELED: return RIN_ERROR_CANCELLED;
#endif
    case EIO:
    case EPIPE:
    case ECONNRESET: return RIN_ERROR_IO;
    case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
    case EINPROGRESS: return RIN_ERROR_WOULD_BLOCK;
#if defined(EBADMSG) && EBADMSG != EINVAL
    case EBADMSG:
#endif
    case EILSEQ: return RIN_ERROR_MALFORMED;
#if defined(ESTALE) && ESTALE != EINVAL
    case ESTALE: return RIN_ERROR_STALE;
#endif
#if defined(ENOTSUP) && ENOTSUP != EINVAL
    case ENOTSUP: return RIN_ERROR_UNSUPPORTED;
#endif
    case ENODEV: return RIN_ERROR_DEVICE_LOST;
    case EINTR: return RIN_ERROR_INTERRUPTED;
    default: return RIN_ERROR_UNKNOWN;
    }
}

static uint32_t kind_from_rin_result(int64_t code)
{
    switch (code) {
    case 0: return RIN_ERROR_SUCCESS;
    case -1: return RIN_ERROR_INVALID_ARGUMENT;
    case -2: return RIN_ERROR_NOT_FOUND;
    case -3: return RIN_ERROR_IO;
    case -4: return RIN_ERROR_PERMISSION_DENIED;
    case -5: return RIN_ERROR_RESOURCE_EXHAUSTED;
    case -6: return RIN_ERROR_ALREADY_EXISTS;
    case -7: return RIN_ERROR_BUSY;
    case -8: return RIN_ERROR_TIMEOUT;
    case -9: return RIN_ERROR_UNSUPPORTED;
    case -10:
    case -11: return RIN_ERROR_INVALID_ARGUMENT;
    case -12: return RIN_ERROR_RESOURCE_EXHAUSTED;
    case -13: return RIN_ERROR_WOULD_BLOCK;
    case -14: return RIN_ERROR_INTERRUPTED;
    case -15: return RIN_ERROR_RESOURCE_EXHAUSTED;
    case -16:
    case -18: return RIN_ERROR_MALFORMED;
    case -17: return RIN_ERROR_INTEGRITY;
    case -19: return RIN_ERROR_RESOURCE_EXHAUSTED;
    case -20: return RIN_ERROR_DEVICE_LOST;
    default: return RIN_ERROR_UNKNOWN;
    }
}

static uint32_t kind_from_sdk(int64_t code)
{
    switch (code) {
    case 0: return RIN_ERROR_SUCCESS;
    case -1: return RIN_ERROR_INVALID_ARGUMENT;
    case -2: return RIN_ERROR_UNSUPPORTED;
    case -3: return RIN_ERROR_NOT_FOUND;
    case -4: return RIN_ERROR_PERMISSION_DENIED;
    case -5: return RIN_ERROR_RESOURCE_EXHAUSTED;
    case -6: return RIN_ERROR_BUSY;
    case -7: return RIN_ERROR_TIMEOUT;
    case -8: return RIN_ERROR_CANCELLED;
    case -9: return RIN_ERROR_IO;
    case -10: return RIN_ERROR_WOULD_BLOCK;
    case -11: return RIN_ERROR_MALFORMED;
    case -12: return RIN_ERROR_STALE;
    case -13: return RIN_ERROR_INTEGRITY;
    default: return RIN_ERROR_UNKNOWN;
    }
}

static uint32_t retryable_kind(uint32_t kind)
{
    return kind == RIN_ERROR_BUSY || kind == RIN_ERROR_TIMEOUT ||
           kind == RIN_ERROR_WOULD_BLOCK || kind == RIN_ERROR_INTERRUPTED;
}

static int canonical_errno_for_kind(uint32_t kind)
{
    switch (kind) {
    case RIN_ERROR_SUCCESS: return 0;
    case RIN_ERROR_INVALID_ARGUMENT: return EINVAL;
    case RIN_ERROR_NOT_FOUND: return ENOENT;
    case RIN_ERROR_PERMISSION_DENIED: return EACCES;
    case RIN_ERROR_RESOURCE_EXHAUSTED: return ENOMEM;
    case RIN_ERROR_ALREADY_EXISTS: return EEXIST;
    case RIN_ERROR_BUSY: return EBUSY;
    case RIN_ERROR_TIMEOUT: return ETIMEDOUT;
    case RIN_ERROR_CANCELLED: return RIN_ERRNO_CANCELLED;
    case RIN_ERROR_IO: return EIO;
    case RIN_ERROR_WOULD_BLOCK: return EAGAIN;
    case RIN_ERROR_MALFORMED: return RIN_ERRNO_BAD_MESSAGE;
    case RIN_ERROR_STALE: return RIN_ERRNO_STALE;
    case RIN_ERROR_REVOKED: return EACCES;
    case RIN_ERROR_UNSUPPORTED: return RIN_ERRNO_NOT_SUPPORTED;
    case RIN_ERROR_INTEGRITY: return RIN_ERRNO_BAD_MESSAGE;
    case RIN_ERROR_DEVICE_LOST: return ENODEV;
    case RIN_ERROR_INTERRUPTED: return EINTR;
    default: return EIO;
    }
}

int rin_error_from_status(uint16_t domain, int64_t code, uint32_t flags,
                          RinErrorV1* output)
{
    uint32_t kind;
    if (output == NULL) return RIN_ERROR_STATUS_INVALID_ARGUMENT;
    clear_error(output);
    if (!domain_valid(domain)) return RIN_ERROR_STATUS_BAD_DOMAIN;
    if (!flags_valid(flags)) return RIN_ERROR_STATUS_BAD_FLAGS;
    if (code == 0) kind = RIN_ERROR_SUCCESS;
    else if ((flags & RIN_ERROR_FLAG_MALFORMED) != 0u) kind = RIN_ERROR_MALFORMED;
    else if ((flags & RIN_ERROR_FLAG_REVOKED) != 0u) kind = RIN_ERROR_REVOKED;
    else if ((flags & RIN_ERROR_FLAG_CANCELLED) != 0u) kind = RIN_ERROR_CANCELLED;
    else if (domain == RIN_ERROR_DOMAIN_ERRNO) kind = kind_from_errno((int)code);
    else if (domain == RIN_ERROR_DOMAIN_RIN_SDK) kind = kind_from_sdk(code);
    else if (domain == RIN_ERROR_DOMAIN_RIN_RESULT ||
             domain == RIN_ERROR_DOMAIN_ENUM_STATUS) kind = kind_from_rin_result(code);
    else if (domain == RIN_ERROR_DOMAIN_NEGATIVE_SYSCALL && code > 0) {
        kind = kind_from_errno((int)code);
    } else {
        kind = RIN_ERROR_UNKNOWN;
    }
    if ((flags & RIN_ERROR_FLAG_RETRYABLE) == 0u && retryable_kind(kind) != 0u) {
        flags |= RIN_ERROR_FLAG_RETRYABLE;
    }
    initialize_error(domain, code, kind, flags, output);
    output->canonical_errno = canonical_errno_for_kind(kind);
    return RIN_ERROR_STATUS_OK;
}

int rin_error_from_code(uint16_t domain, int64_t code, RinErrorV1* output)
{
    return rin_error_from_status(domain, code, 0u, output);
}

int rin_error_from_errno(int error_number, RinErrorV1* output)
{
    if (error_number < 0) {
        clear_error(output);
        return RIN_ERROR_STATUS_INVALID_ARGUMENT;
    }
    return rin_error_from_status(RIN_ERROR_DOMAIN_ERRNO, error_number, 0u,
                                 output);
}

int rin_error_from_negative_syscall(intptr_t result, RinErrorV1* output)
{
    int64_t code;
    if (result >= 0) code = 0;
    else if (result == INTPTR_MIN) code = INT64_MAX;
    else code = -(int64_t)result;
    return rin_error_from_status(RIN_ERROR_DOMAIN_NEGATIVE_SYSCALL, code, 0u,
                                 output);
}

int rin_error_from_win32(uint32_t error_code, RinErrorV1* output)
{
    uint32_t kind;
    if (output == NULL) return RIN_ERROR_STATUS_INVALID_ARGUMENT;
    switch (error_code) {
    case 0: kind = RIN_ERROR_SUCCESS; break;
    case 2:
    case 3:
    case 1168: kind = RIN_ERROR_NOT_FOUND; break;
    case 5: kind = RIN_ERROR_PERMISSION_DENIED; break;
    case 6: kind = RIN_ERROR_INVALID_ARGUMENT; break;
    case 8:
    case 14: kind = RIN_ERROR_RESOURCE_EXHAUSTED; break;
    case 32:
    case 170: kind = RIN_ERROR_BUSY; break;
    case 80: kind = RIN_ERROR_ALREADY_EXISTS; break;
    case 995:
    case 1223: kind = RIN_ERROR_CANCELLED; break;
    case 996: kind = RIN_ERROR_WOULD_BLOCK; break;
    case 122: kind = RIN_ERROR_RESOURCE_EXHAUSTED; break;
    case 995 + 2: kind = RIN_ERROR_IO; break;
    case 1460: kind = RIN_ERROR_TIMEOUT; break;
    case 1117: kind = RIN_ERROR_IO; break;
    case 1167: kind = RIN_ERROR_DEVICE_LOST; break;
    case 87: kind = RIN_ERROR_INVALID_ARGUMENT; break;
    default: kind = RIN_ERROR_UNKNOWN; break;
    }
    initialize_error(RIN_ERROR_DOMAIN_WIN32, error_code, kind,
                     retryable_kind(kind) ? RIN_ERROR_FLAG_RETRYABLE : 0u,
                     output);
    output->canonical_errno = canonical_errno_for_kind(kind);
    return RIN_ERROR_STATUS_OK;
}

int rin_error_is_success(const RinErrorV1* error)
{
    return rin_error_validate(error) == RIN_ERROR_STATUS_OK &&
           error->kind == RIN_ERROR_SUCCESS;
}

int rin_error_is_retryable(const RinErrorV1* error)
{
    return rin_error_validate(error) == RIN_ERROR_STATUS_OK &&
           ((error->flags & RIN_ERROR_FLAG_RETRYABLE) != 0u ||
            retryable_kind(error->kind) != 0u);
}

int rin_error_is_cancellation(const RinErrorV1* error)
{
    return rin_error_validate(error) == RIN_ERROR_STATUS_OK &&
           (error->kind == RIN_ERROR_CANCELLED ||
            (error->flags & RIN_ERROR_FLAG_CANCELLED) != 0u);
}

int rin_error_is_malformed(const RinErrorV1* error)
{
    return rin_error_validate(error) == RIN_ERROR_STATUS_OK &&
           (error->kind == RIN_ERROR_MALFORMED ||
            (error->flags & RIN_ERROR_FLAG_MALFORMED) != 0u);
}

int rin_error_is_revoked(const RinErrorV1* error)
{
    return rin_error_validate(error) == RIN_ERROR_STATUS_OK &&
           (error->kind == RIN_ERROR_REVOKED ||
            (error->flags & RIN_ERROR_FLAG_REVOKED) != 0u);
}

int rin_error_to_errno(const RinErrorV1* error)
{
    if (rin_error_validate(error) != RIN_ERROR_STATUS_OK) return EINVAL;
    return error->canonical_errno;
}

const char* rin_error_kind_name(uint32_t kind)
{
    static const char* const names[] = {
        "success", "invalid_argument", "not_found", "permission_denied",
        "resource_exhausted", "already_exists", "busy", "timeout",
        "cancelled", "io", "would_block", "malformed", "stale", "revoked",
        "unsupported", "integrity", "device_lost", "interrupted", "unknown"
    };
    return kind <= RIN_ERROR_UNKNOWN ? names[kind] : "unknown";
}
