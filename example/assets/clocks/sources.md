# Clock Fonts

Google Sans Flex is used only by the clock widget examples. The retained static
weight 500 was downloaded through the Google Fonts CSS API on 2026-10-17:

https://fonts.googleapis.com/css2?family=Google+Sans+Flex:wght@500;900&display=swap

Upstream: https://github.com/googlefonts/googlesans-flex

License: SIL Open Font License 1.1, included in `OFL.txt` and the font metadata.
There are no Reserved Font Names in the distributed license.

The medium font retains Latin, Greek, Cyrillic, available Arabic characters,
general punctuation and currency characters. Characters absent from this subset
use Qt's normal font fallback. The font contains no variable axes. Numeral's
fixed dial numbers use vector paths instead of a second font.

Generated with fontTools 4.67.0:

```sh
pyftsubset google-sans-flex-medium.ttf \
  --unicodes='U+0000-024F,U+0370-052F,U+0600-06FF,U+2000-206F,U+20A0-20CF' \
  --layout-features='*' --output-file=GoogleSansFlex-Medium.ttf
```

# Drawable References

`ClockWidgetHand.qml` and `ClockWidgetNumerals.qml` adapt drawable geometry from
the user-supplied Google Clock APK, version `9.1 (965977594)`, package
`com.google.android.deskclock`. The input archive contains `base.apk` and
`split_config.xxhdpi.apk`; the vectors are in `base.apk`.

- `res/drawable-v31/clock_analog_numeral_dial.xml`: fixed 12/3/6/9 outlines in a
  182-unit viewport. The background is deliberately not copied.
- `clock_analog_{hour,minute,second}.xml` and their `_cutout.xml` drawables:
  Scallop's wide hands and orbiting second dot.
- `clock_analog_simple_{hour,minute,second}.xml` and their `_cutout.xml`
  drawables: the other faces' hands, including the second hand's integral hub.
- `clock_analog_day_hour.xml` and its `_cutout.xml`: Day's outlined hour hand.

Hand vectors retain their own viewports and intrinsic sizes relative to the
380dp reference dial. Android's `InsetDrawable` first reconstructs an intrinsic
outer rectangle from the child size and inset fractions; `AnalogClock` centers
that rectangle on the pivot. The QML example uses the equivalent continuous
geometry, without density-specific integer rounding. It does not multiply the
insets directly by the dial size. Rotation consumes `ClockBase` angles; paths
are independent of time and are not regenerated on each tick.

Only the supplied `OFL.txt` covers the Google Sans Flex font. It does not establish
a license for the APK's artwork.

# Visual References

- Official analog widgets: https://blog.google/products-and-platforms/platforms/android/widgets-just-got-better-android-12/
- Analog face selector and solid/transparent digital widgets: https://www.xatakandroid.com/aplicaciones-android/reloj-google-7-1-estrena-nuevo-diseno-wear-os-widgets-android-12
- Google Clock 8.1 retained these widgets during the Android 16-era app redesign: https://9to5google.com/2025/08/22/google-clock-material-3-expressive/

The example recreates their composition using `ClockBase` and Material shapes.
Screenshot dimensions are visual references, not Android dp specifications.
Clock and alarm data remain explicit inputs; the example does not schedule alarms.
The stacked preview retains the existing vertical layout; the linked pictures do
not establish its complete responsive behavior. The theme supplies colors, and
the four-sided-cookie outline is used for the widget named Clover rather than
the unrelated four-leaf-clover shape.
