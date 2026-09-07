#pragma once

// Read-only filesystem over the asset archive linked into the child binary.
// See AssetDevice.cpp for why this exists and how it is wired up.

// Registers the device with the C library. It becomes the current drive, so the
// game's relative asset paths ("cards/card_1_png.grf") resolve to it with no
// changes anywhere else. Idempotent. Returns false if the linked archive is
// missing or corrupt, or if libnds refused the device.
bool AssetDeviceMount();

// Number of files in the archive, or 0 if it isn't usable. For diagnostics.
int AssetDeviceFileCount();
