#include "field_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// DateTime Parsing
// ============================================================================

ORM_DateTime* ORM_ParseDateTime(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    ORM_DateTime* dt = (ORM_DateTime*)calloc(1, sizeof(ORM_DateTime));
    if (dt == NULL) {
        return NULL;
    }
    
    // Try to parse full datetime: "YYYY-MM-DD HH:MM:SS"
    int parsed = sscanf(str, "%d-%d-%d %d:%d:%d",
                       &dt->year, &dt->month, &dt->day,
                       &dt->hour, &dt->minute, &dt->second);
    
    if (parsed == 6) {
        // Full datetime parsed successfully
        return dt;
    }
    
    // Try to parse date only: "YYYY-MM-DD"
    parsed = sscanf(str, "%d-%d-%d", &dt->year, &dt->month, &dt->day);
    
    if (parsed == 3) {
        // Date parsed successfully, time defaults to 00:00:00
        dt->hour = 0;
        dt->minute = 0;
        dt->second = 0;
        return dt;
    }
    
    // Try to parse time only: "HH:MM:SS"
    parsed = sscanf(str, "%d:%d:%d", &dt->hour, &dt->minute, &dt->second);
    
    if (parsed == 3) {
        // Time parsed successfully, date defaults to epoch
        dt->year = 1970;
        dt->month = 1;
        dt->day = 1;
        return dt;
    }
    
    // Try alternative format with microseconds: "YYYY-MM-DD HH:MM:SS.mmmmmm"
    parsed = sscanf(str, "%d-%d-%d %d:%d:%d.%d",
                   &dt->year, &dt->month, &dt->day,
                   &dt->hour, &dt->minute, &dt->second, &dt->microsecond);
    
    if (parsed == 7) {
        return dt;
    }
    
    // Parsing failed
    free(dt);
    return NULL;
}

// ============================================================================
// DateTime Formatting
// ============================================================================

int ORM_FormatDateTime(const ORM_DateTime* dt, const char* format, char* buffer, size_t size) {
    if (dt == NULL || buffer == NULL || size == 0) {
        return -1;
    }
    
    // Validate datetime
    if (!ORM_ValidateDateTime(dt)) {
        return -1;
    }
    
    // If no format specified, use default ISO format
    if (format == NULL || strcmp(format, "") == 0) {
        if (dt->microsecond > 0) {
            return snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%06d",
                          dt->year, dt->month, dt->day,
                          dt->hour, dt->minute, dt->second, dt->microsecond);
        } else {
            return snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
                          dt->year, dt->month, dt->day,
                          dt->hour, dt->minute, dt->second);
        }
    }
    
    // Convert ORM_DateTime to struct tm for strftime
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(struct tm));
    
    tm_time.tm_year = dt->year - 1900;
    tm_time.tm_mon = dt->month - 1;
    tm_time.tm_mday = dt->day;
    tm_time.tm_hour = dt->hour;
    tm_time.tm_min = dt->minute;
    tm_time.tm_sec = dt->second;
    
    size_t result = strftime(buffer, size, format, &tm_time);
    return (result > 0) ? (int)result : -1;
}

// ============================================================================
// DateTime Conversion
// ============================================================================

time_t ORM_DateTimeToTimestamp(const ORM_DateTime* dt) {
    if (dt == NULL || !ORM_ValidateDateTime(dt)) {
        return -1;
    }
    
    struct tm tm_time;
    memset(&tm_time, 0, sizeof(struct tm));
    
    tm_time.tm_year = dt->year - 1900;
    tm_time.tm_mon = dt->month - 1;
    tm_time.tm_mday = dt->day;
    tm_time.tm_hour = dt->hour;
    tm_time.tm_min = dt->minute;
    tm_time.tm_sec = dt->second;
    tm_time.tm_isdst = -1; // Let mktime determine DST
    
    return mktime(&tm_time);
}

ORM_DateTime* ORM_TimestampToDateTime(time_t timestamp) {
    if (timestamp < 0) {
        return NULL;
    }
    
    ORM_DateTime* dt = (ORM_DateTime*)calloc(1, sizeof(ORM_DateTime));
    if (dt == NULL) {
        return NULL;
    }
    
    struct tm* tm_time = localtime(&timestamp);
    if (tm_time == NULL) {
        free(dt);
        return NULL;
    }
    
    dt->year = tm_time->tm_year + 1900;
    dt->month = tm_time->tm_mon + 1;
    dt->day = tm_time->tm_mday;
    dt->hour = tm_time->tm_hour;
    dt->minute = tm_time->tm_min;
    dt->second = tm_time->tm_sec;
    dt->microsecond = 0;
    
    return dt;
}

// ============================================================================
// DateTime Utility Functions
// ============================================================================

void ORM_FreeDateTime(ORM_DateTime* dt) {
    if (dt != NULL) {
        free(dt);
    }
}

ORM_DateTime* ORM_GetCurrentDateTime(void) {
    time_t now = time(NULL);
    return ORM_TimestampToDateTime(now);
}

// ============================================================================
// DateTime Arithmetic (Bonus Functions)
// ============================================================================

/**
 * Compare two datetime values
 * @return -1 if dt1 < dt2, 0 if equal, 1 if dt1 > dt2
 */
int ORM_CompareDateTime(const ORM_DateTime* dt1, const ORM_DateTime* dt2) {
    if (dt1 == NULL || dt2 == NULL) {
        return 0;
    }
    
    if (dt1->year != dt2->year) return (dt1->year < dt2->year) ? -1 : 1;
    if (dt1->month != dt2->month) return (dt1->month < dt2->month) ? -1 : 1;
    if (dt1->day != dt2->day) return (dt1->day < dt2->day) ? -1 : 1;
    if (dt1->hour != dt2->hour) return (dt1->hour < dt2->hour) ? -1 : 1;
    if (dt1->minute != dt2->minute) return (dt1->minute < dt2->minute) ? -1 : 1;
    if (dt1->second != dt2->second) return (dt1->second < dt2->second) ? -1 : 1;
    if (dt1->microsecond != dt2->microsecond) return (dt1->microsecond < dt2->microsecond) ? -1 : 1;
    
    return 0;
}

/**
 * Create a copy of a datetime value
 */
ORM_DateTime* ORM_CopyDateTime(const ORM_DateTime* dt) {
    if (dt == NULL) {
        return NULL;
    }
    
    ORM_DateTime* copy = (ORM_DateTime*)malloc(sizeof(ORM_DateTime));
    if (copy == NULL) {
        return NULL;
    }
    
    memcpy(copy, dt, sizeof(ORM_DateTime));
    return copy;
}
