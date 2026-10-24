# Debug screen awake — 2026-10-24

User explicitly requested restoring prevention of automatic screen lock while
debugging. ADB verified prior stay_on_while_plugged_in=15 was already active;
screen_off_timeout was600000ms. Last recorded sleep reason was power_button,
not idle timeout. Device-admin timeout limit was not enforced.

Applied svc power stayon true and screen_off_timeout=2147483647ms (maximum
signed32-bit setting). Readback and dumpsys confirm both settings, Awake and
mStayOn=true with USB power. This prevents idle screen-off during debugging,
including a long screen timeout if USB power temporarily disappears. It does
not disable PIN, keyguard, manual power-button lock or authentication policy.
No claim that this can unlock an already locked device.

To restore the prior ordinary screen timeout after debugging:
adb shell settings put system screen_off_timeout 600000
Prior stay-on setting was15; it was not newly introduced in this turn.
Do not change the user's PIN or lock_screen_lock_after_timeout (still5000).
No Steam, APK, VM or production runtime changes.
