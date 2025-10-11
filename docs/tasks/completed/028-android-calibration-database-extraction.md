# Task 028: Android Calibration Database Extraction

**Status:** completed  
**Priority:** high  
**Phase:** VIII - VM Integration & Android Analysis Integration  
**Assigned:** AI Agent  
**Created:** 2025-10-11  
**Completed:** 2025-10-11  
**Context:** `ai/contexts/android-calibration-integration.md`

## Objective

Extract and analyze tvpq.db SQLite database from Android firmware to understand factory display calibration parameters and picture mode settings.

## Prerequisites

- [x] Task 032 completed (Android firmware analysis)
- [x] super.img extracted with binwalk
- [x] tvpq.db located at `firmware/extractions/super.img.extracted/33128670/rootfs/etc/tvconfig/tvpq.db`

## Acceptance Criteria

- [x] Database structure analyzed (3 tables identified)
- [x] Picture_Mode table schema documented
- [x] Gamma_Point table schema documented
- [x] White_Balance_Mode table schema documented
- [x] Factory calibration values extracted
- [x] Picture mode names identified (standard, cinema, vivid, game, hdr, computer, custom)

## Implementation Steps

### 1. Analyze Database Structure
Used hexdump and strings to extract database schema without sqlite3 dependency:
- Identified 3 main tables
- Extracted CREATE TABLE statements
- Documented primary keys and constraints

### 2. Extract Picture_Mode Data
Found 20 picture mode presets with parameters:
- brightness, contrast, saturation, hue (0-100 range)
- sharpness (0-100 range, default 100)
- tnr/snr (noise reduction, default 8)
- backlight (0-100, default 29)
- colortemperature, gamma, dci, blackextension
- dynamic_backlight mode

### 3. Extract White_Balance Data
Documented RGB calibration structure:
- Per-input source (tvin) calibration
- RGain/GGain/BGain adjustments
- ROffset/GOffset/BOffset corrections
- Multiple color temperature modes (1-4)

### 4. Document Gamma Correction
Identified gamma LUT structure:
- Point-based lookup table
- id/value pairs for correction curve

### 5. Identify Factory Defaults
Standard mode (tvin=8, mode=1) defaults:
- All picture params: 50 (0x32)
- Sharpness: 100 (0x64)
- Noise reduction: 8 each
- Backlight: 29 (0x1d)

## Quality Validation

- [x] All table schemas documented
- [x] Factory calibration values extracted
- [x] Multiple picture modes identified
- [x] Data structures defined for Linux integration
- [x] Integration context document updated

## Next Task Dependencies

- Task 028-2: Update MIPS Driver with Android Memory Layout
- Task 028-3: Update Device Tree Panel Parameters
- Task 028-4: Create Android Calibration Integration Context (completed concurrently)

## Notes

- Database is 16KB SQLite3 format
- Contains factory-calibrated values that should be preserved exactly
- Standard mode provides baseline calibration for Linux integration
- Advanced modes (cinema, vivid, game, hdr) can be implemented later
- Some calibration may be handled by MIPS co-processor, not ARM Linux directly
