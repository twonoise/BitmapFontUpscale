# BitmapFontUpscale
Research: Bitmap font upscale

Description
-----------

There are matrix (pixel) based screen displays are extremely widely used today; one may note that it is almost only type of displays produced currently. Vector-based display technologies today are very special and quite expensive.

One may also note that screen display resolution in pixels increases with years.
This leads to increasing demand of lagre-sized pixel-precise fonts. The shape (outline) based, or `.ttf`, fonts are can be rendered to bitmap form, but result will never be perfect: it is impossible for machine to know where to place boundary pixels for perfect result, and only human can do that. And when `.ttf` are used for what they supposed to, paper printing, this imperfection never noticeable for human eye, as 300 dpi printers were readily available at first wave of `.ttf`s arrival. 

But, someone wrongly try to use `.ttf` for matrix displays one day, and this poor practice becomes treated as normal thing by many new or non-experienced desktop users, so becomes widely spreaded today. Insane amount of human power then was consumed last half of century during countless tries to make _hinting_ and _antialiasing_ pseudo-perfection technologies to make use `.ttf` for matrix displays more "perfect". With respect to these brave people, but really no even one bit of success there, as _this_ is theoretically impossible for machine <sup>[Note]</sup>. Only loss, because antialiasing, and especially RGB subpixeling version of it, is bad for screenshots for web articles, for editing, printed copies; screen reading (recognizing of chars when cursor copy not work); LED panels; industrial (low DPI) laminated label printing; e-paper displays; embedded platforms; low power SPI/I2C displays. And this becomes even worse for small sized fonts. CPU time and RAM usage also rarely benefits from that.

> [!Tip]
> Try to render _any_ symmetrical letter with _any_ popular `.ttf` font and _required_ size, and see if result is perfect.

> [!Note]
> That does not mean machines can't generate perfect results at all. This research was created and intended to show they _can_.

Old desktop operating systems were made by professionals. They were, of course, aware of this problem: their fonts always comes in screen-paper pairs of 1) human-created, guaranteed to be pixel-perfect at any case, bitmap fonts in a number of sizes (or only just this part), plus 2) complementary `.ttf` part, which used for display for larger sizes, and for hardcopy for any size, of letters. This only correct behaviour is mostly lost, or was intentionally thrown away, today. I suspect that newbies are never seen those word processing systems, so just don't know how it should look when correctly made.
Another reason why `.ttf` fonts are impossibly to be perfectly rendered (here and after, "perfectly" means for matrix display unit, not for paper print, where they are already perfect), are floating point format of their data. Compared with _bitmap_ fonts, and also with another important type, _Hershey_ (aka stroke, or vector, or Borland's .bgi) type: these both are perfect, as they are integer based.
3rd reason is not excellent quality of most shape-based (`.ttf`) fonts itself: no grid used, at least. _I'm an Artist, that's how I see it! Your boring machines and displays and dpi are not my problem at all!_

To restore the correct behaviour, we'll need to know there are two problems. One is bitmap font support in modern desktop OS: there will be separate research. Other one is limited set of large bitmap fonts available, and the purpose of this research is to somewhat overcome it.

While there are a number of bitmapped fonts are exist, they are mostly small sized. It is not easy to draw a large bitmap font. And it's very hard (i mean, _really_) to (perfectly) upscale existing one. But, looks like i found some of required spices for our witch's potion cauldron, so we can try now to do some magic. Spells will be (very) different per font, per its style (bold or not) and size, and also per scale ratio. Expected to be hard... but, let's try.

First, let's look at existing attempts.

Most obvious is just upscale of glyph bitmaps. As with any bitmap, this is possible with either *straight* way, and *smart* (guess missed pixels) way.
*Straight* upscale is done using

* bdfresize https://github.com/ntwk/bdfresize/ , and
* bdf2x https://github.com/Francesco149/bdf2x ;

one may note that former allows non-integer upscaling with good guess logic for that case.
> [!Note]
> We will use only integer, 1:N upscale here in our research.

*Smart* **image** upscale is tricky. It can be possible using bicubic or other non-linear upscaler. I am not found already existing **font** scalers based on it. I'd have some success with it using my 8-16x supersampling method, but not all chars are good, and almost not useful above 2x upscaling.

It can be also possible using 1) extraction of outline (which means, and is, bitmap -> outline font conversion), then 2) smoothing angles, then 3) reverse convert back to bitmap. There is:

* vfontas (https://github.com/consoleet/consoleet-utils, https://inai.de/projects/consoleet/terminus)

which does exactly that, but only steps 1 and 2, but not 3 (back-conversion). There is impossible to pixel-precisely (_exactly_ match the original bitmap) render `.ttf` outside of program generates it (at least, i can't). (It never means that `.ttf` file itself is bad!) But one more thing is worse: only 45 degree angles are smoothed, but not ~30 deg ones (2:1 pixel step) (btw, latter are most hard thing to smooth at all by generic image upscalers, we will see it later). This 3-steps approach also may have perfect 2x and 4x upscaling in theory, but not 3x, due to its specific requirement for upscaled 45 dergee lines thickness.

There is also this magic:

* Scale2x https://www.scale2x.it/ See also  https://legacy.imagemagick.org/Usage/resize/#magnify

It have other purpose, but still useful for 1-bit images, and its special guessing logic can be interesting. I found that only triple upscale with it (as per magick example) is somewhat useful for fonts; but there are errors in algorithm gives annoying inverse-X-squared artifacts, so it is rarely useful for us, or, after-editing by hands will be need; but, most attractive is it is only upscaler which make correct 30 degree lines upscale out-of-box (however we rarely can use that, sadly, due to imperfections). It is also rounds glyph stroke ends; and while it is considered unacceptable here due to we need to respect and preserve font style, but someone can find rounded fonts pretty.

So, as no one existing code fully works, we should create something new.

But before we write something, we should ensure if correct upscale is possible at all.
Let's manually create this 1-bit test bitmap using Terminus font with various sizes...

./test.png

...and try to 2x scale it.
I will omit multiple previous research attempts here (btw, some are can be found at my old posts everywhere), and show final spell i found for it:

    gegl test.png -o out1.png -- scale-ratio x=2 y=2 sampler=nearest mean-curvature-blur iterations=1 threshold value=0.35

> [!Note]
> Btw, `scale-ratio x=2 y=2 sampler=nearest` part here is straight upscaler, available also everywhere else.

Here is key spice for our potion is  **M e a n   C u r v a t u r e   f i l t e r** . From some well known image processing tools, only `gegl` (and `gimp` which use it) not only have this filter, but correct implementation of it. <sub>Example of pixel-incorrect one in my tests, is https://github.com/YuanhaoGong/CurvatureFilter.</sub>
One may note that *Mean Curvature filter* have quite different purpose; this makes our work harder, in terms of issues reporting, if any.
So, use our spell and examine carefully **top half** of result (*thin/small* font). Everything except 30 degree lines seems to be perfect. This gives us a chance that our work can be succeeded. Btw, if we try to adopt this spell for **bottom half** of image, it will be

    gegl test.png -o out2.png -- scale-ratio x=2 y=2 sampler=nearest mean-curvature-blur iterations=1 threshold value=0.2

and is not precisely correct any more: it have added *corner pixels*. We don't want it, and it is very evil effect to fight with. One approach to remove can be like

    magick out2.png -depth 1 -morphology Thinning '5>:1,1,1,1,- 1,1,1,1,- 1,1,1,0,0 1,1,0,0,0 -,-,0,0,0' out2a.png

but it also etch some correct pixels. We'll return to it later. Now it's time and reasons to set up our...

Work flow
=========
There are two possible approaches.
* One is per-glyph processing, as `bdfresize` and others does.
* Other is render one whole bitmap with all glyphs, process it, then disassemble the puzzle back to its pieces.

Per-glyph work can be acceptable for simple, or at least stable, processing chain for all fonts; but as we see at simple example above, it is rarely possible, we will use many different tools. Also, batch processing of small images is (often very much) inefficient. Btw, i've started from that, and spent many time trying to have success with it.

Whole bitmap process is at a glance and very efficient. But there is no tools that allow mentioned prosess chain, because it requires to restore metadata for each glyph, which of course can't be stored in a bitmap. 
So we will try:
1) Render one whole bitmap with all glyphs—there is standard tool(s) for that;
2) Process bitmap by hand—steps can't be automated, they are differs per font style, font size, and scaling size, we'd should support all these combinations;
3) Reconstruct the font, with the trick: we will use `bdf2x`, but modify it, so it will read our whole resulting bitmap instead of its per-glyph process, thus save all font metadata.

> [!Note]
> This workflow tested only for *monospaced* fonts.

> [!Note]
> We will prefer existing solutions/code here at any cost, even when commands looks overcompliceted a bit, to make things more stable and understandeable. We will introduce own code only if it's impossible to solve else.

Step One
--------
We need to render font to one whole bitmap, with defined a) gap, and b) row glyphs quantity. This tool fits well:

* bdf2bmp https://github.com/itouhiro/bdf2bmp

But it produces `bmp`s i can't open with any tool. So it was modified to produce raw output, we then use `magick` to convert to any regular format; and, simplified some things. I name it `bdf2image`. Please see more at *Code* section.

> [!Note]
> One may note that we treat `.pcf` and `.bdf` as same formats. But not `.pcf.gz`: these need fo be unzipped first; no need to gzip it again then, it will work as is.

    gcc -o bdf2image bdf2image.c
    bdf2image ter-x24nx1.pcf      # Or original name, ter-x24n.pcf

Note that default gap (guard or grid) will be **2 px**. We can't work without gap: it can look overcomplicated to chop-crop-chop sequence below to remove gaps, but gaps are required to work as defined out-of-area (virtual) pixels during our magic upscale math; so should be black, or, to help see boundaries for debug, i use near black.

As suggested by `bdf2image`
    magick -size 450x1146 -depth 8 gray:out.raw ter-x24nx1.png

Step Two (example)
--------

    gegl ter-x24nx1.png -o ter-x24nx2.png -- scale-ratio x=2 y=2 sampler=nearest mean-curvature-blur iterations=1 threshold value=0.35

Make few example-like (not final) manual edits in it.

Note that we have gap=2, scale=2, our 24 px font is 12x24 sized.
    s=2
    g=$((2*s))

Test how they looks:
> [!Warning]
> Generation of a lot of files!

    magick ter-x24nx2.png -gravity SouthEast -chop "$g"x"$g" -crop $((12*s+g))x$((24*s+g)) -gravity NorthWest -chop "$g"x"$g" +repage -depth 1 /tmp/ter-x24nx2-%04d.png

Step Three
----------
Now try to pack it all back to `.bdf` format. <sub>I can't use binary pipe in correct C way, `getchar()` not work, so i use Ascii (hex) one in `bdfgrow`, see below.</sub>

    s=2                        # Scale
    g=$((2*s))                 # Gap (scaled)

    magick ter-x24nx2.png -gravity SouthEast -chop "$g"x"$g" -crop $((12*s+g))x$((24*s+g)) -gravity NorthWest -chop "$g"x"$g" +repage -depth 1 gray:- | xxd -p -c 0 | ./bdfgrow ./ter-x24nx1.pcf > ter-x24nx2.pcf

Try to open it now with `fontforge` or other renderer. We have thus our work flow complete and tested. Note that it is not final font shape, we only test if it all will work here. Btw, what'w wrong with this font? Most notably it is 30 degree lines found at Y or X-like letters. We'll need to fix these, so we use...


Per-font Step Two
=================
We will use `Terminus`, as it 1) originated in industry hardened `.pcf`/`.bdf`, but not `.otb` format, and 2) covers more than octave of sizes, which, using integer 1:N scaling, will produce full gapless set of useful sizes; we will try to create up to 128 (N=4).

> [!Note]
> "_not_ `.otb`" is not an error: i'll explain it in complementary research, why `.otb` (which is also bitmapped, not outline-based) is bad here.

I collect here best processing i can obtain with standard tools. This does not mean that it can't be made even better (lesser hand after-editing). Feedback welcome.

> [!Note]
> Required `.png`'s are taken from Step One, which is same for all fonts.

## Thin (normal) font

### 2x

    gegl ter-x24nx1.png -o ter-x24nx2-pre.png -- scale-ratio x=2 y=2 sampler=nearest mean-curvature-blur iterations=1 threshold value=0.35

And special powerful spell for 30 degree corners fix.

    magick ter-x24nx2-pre.png -depth 1 -morphology Thinning '7>:-,-,-,1,-,0,- -,0,0,1,1,0,- -,0,0,1,1,0,- -,0,0,1,1,1,- -,0,0,0,1,1,- -,0,0,0,0,1,- -,0,0,0,0,1,-' -morphology Thinning '7>:-,0,-,1,-,-,- -,0,1,1,0,0,- -,0,1,1,0,0,- -,1,1,1,0,0,- -,1,1,0,0,0,- -,1,0,0,0,0,- -,1,0,0,0,0,-' ter-x24nx2.png

### 3x
...is way harder than 2x. Unloke 2x, here is no threshold value exist for both smooth tilted lines but not add corner pixels. How we will fight it?

Here is just plain scale:

    gegl ter-x24nx1.png -s 3 -o ter-x24nx3-plain.png -- threshold value=0.5

Then, try to get _only_ corner pixels:

    gegl ter-x24nx3-plain.png -o ter-x24nx3-w-corners.png -- mean-curvature-blur iterations=2 threshold value=0.45

Guess what's next? Yes, time to subtract two results:

    compare -compose subtract ter-x24nx3-w-corners.png ter-x24nx3-plain.png - | magick - -alpha off -threshold 50% ter-x24nx3-corners-only.png

Now one more spell. It is similar to one we already know, but this one has very sensitive value, will not work outside of 0.10...0.11 interval:

    gegl ter-x24nx3-plain.png -o ter-x24nx3-w-corners-and-lines.png -- mean-curvature-blur iterations=2 threshold value=0.105

Now the final spell!

    compare -compose subtract ter-x24nx3-w-corners-and-lines.png ter-x24nx3-corners-only.png - | magick - -alpha off -threshold 50% ter-x24nx3-pre.png

One may note that some chars like 1, 4, looks like have etched out near top (where 45 degree line is). But note that this is the nature of thece chars, as it can be seen on 3x plain-upscaled version.

> [!Tip]
> Use `gegl ter-x24nx3-plain.png` or `magick display ter-x24nx3-plain.png` to show images.

Note that 30 dergee lines are wrong. Let's try one we already know:

    magick ter-x24nx3-pre.png -depth 1 -morphology Thinning '7>:-,-,-,1,-,0,- -,0,0,1,1,0,- -,0,0,1,1,0,- -,0,0,1,1,1,- -,0,0,0,1,1,- -,0,0,0,0,1,- -,0,0,0,0,1,-' -morphology Thinning '7>:-,0,-,1,-,-,- -,0,1,1,0,0,- -,0,1,1,0,0,- -,1,1,1,0,0,- -,1,1,0,0,0,- -,1,0,0,0,0,- -,1,0,0,0,0,-' ter-x24nx3.png

It is not work, because it search for this kernel: (`-` are don't cares)
<tt>
-,-,-,1,-,0,-
-,0,0,1,1,0,-
-,0,0,1,1,0,-
-,0,0,*1*,1,1,-     Note central pixel, it will be etched.
-,0,0,0,1,1,-
-,0,0,0,0,1,-
-,0,0,0,0,1,-
</tt>


...but also is etching only pixels (which is `Thinning` in our spell) is not enough, as we also need to add missing pixels.

Will it work like that?
<tt>
*Thinning* kernel
-,-,-,1,1,1,-
-,0,0,1,1,1,-
-,0,0,1,1,1,-
-,0,0,*1*,1,1,-      *1* -> *0*
-,0,0,0,1,1,-
-,0,0,0,0,1,-
-,0,0,0,0,0,-

*Thicken* kernel
-,-,1,1,1,1,-
-,0,1,1,1,1,-
-,0,0,1,1,1,-
-,0,0,*0*,1,1,-      *0* -> *1*
-,0,0,0,1,1,-
-,0,0,0,1,1,-
-,0,0,0,-,1,-
</tt>

    magick ter-x24nx3-pre.png -depth 1 \
       -morphology Thinning '7>:-,-,-,1,1,1,- -,0,0,1,1,1,- -,0,0,1,1,1,- -,0,0,1,1,1,- -,0,0,0,1,1,- -,0,0,0,0,1,- -,0,0,0,0,0,-' \
       -morphology Thinning '7>:-,1,1,1,-,-,- -,1,1,1,0,0,- -,1,1,1,0,0,- -,1,1,1,0,0,- -,1,1,0,0,0,- -,1,0,0,0,0,- -,0,0,0,0,0,-' \
       -morphology Thicken  '7>:-,-,1,1,1,1,- -,0,1,1,1,1,- -,0,0,1,1,1,- -,0,0,0,1,1,- -,0,0,0,1,1,- -,0,0,0,1,1,- -,0,0,0,-,1,-' \
       -morphology Thicken  '7>:-,1,1,1,1,-,- -,1,1,1,1,0,- -,1,1,1,0,0,- -,1,1,0,0,0,- -,1,1,0,0,0,- -,1,1,0,0,0,- -,1,-,0,0,0,-' ter-x24nx3.png

Yes, looks like this spell is more powerful now, here it is:

UWXY.png

Btw, isn't our spell _too_ powerful? Let's see if we added or etched some pixels incorrectly:

    gegl-imgcmp ter-x24nx3-pre.png ter-x24nx3.png

> [!Note]
> Unlike `compare subtract`ion, this one counts both missed and extra pixels, so order of arguments is irrelevant.

...and check _throughly_ `...diff.png` file created. Believe or not, but our spell also unintentionally rounded 45 deg. to straight lines joints, and make it in most pretty way! _What's a real magic!_ Really, i did not planned that... Btw, if this effect is bad for you, let me know, i'll try to narrower the kernels.

Btw, manual edit still unavoidable. Some chars like V have more than 2:1 (less tilted) lines, and either even more strong spell need, or, manual edit of these; also, some places like mentioned earlier 1, 4 digits, arrows and braces, and some more. Also, (manually) fix "1, 4" problem is may be better before latter long spell.

### 4x
For this scale we'll use our 2x version after final manual touch on it. Flow is similar to 3x, but differs in that it use 9x9 etching kernel.

    gegl ter-x24nx2.png -s 2 -o ter-x24nx4-plain.png -- threshold value=0.5
    gegl ter-x24nx4-plain.png -o ter-x24nx4-w-corners.png -- mean-curvature-blur iterations=2 threshold value=0.45
    compare -compose subtract ter-x24nx4-w-corners.png ter-x24nx4-plain.png - | magick - -alpha off -threshold 50% ter-x24nx4-corners-only.png
    gegl ter-x24nx4-plain.png -o ter-x24nx4-w-corners-and-lines.png -- mean-curvature-blur iterations=2 threshold value=0.105
    compare -compose subtract ter-x24nx4-w-corners-and-lines.png ter-x24nx4-corners-only.png - | magick - -alpha off -threshold 50% ter-x24nx4-pre.png
    magick ter-x24nx4-pre.png -depth 1 -morphology Thinning '9>:-,-,-,-,1,1,1,-,- -,-,-,-,1,1,1,-,- -,-,0,0,1,1,1,-,- -,-,0,0,1,1,1,-,- -,-,0,0,1,1,1,-,- -,-,0,0,0,1,1,-,- -,-,0,0,0,0,1,-,- -,-,0,0,0,0,0,-,- -,-,0,0,0,0,0,-,-' -morphology Thinning '9>:-,-,1,1,1,-,-,-,- -,-,1,1,1,-,-,-,- -,-,1,1,1,0,0,-,- -,-,1,1,1,0,0,-,- -,-,1,1,1,0,0,-,- -,-,1,1,0,0,0,-,- -,-,1,0,0,0,0,-,- -,-,0,0,0,0,0,-,- -,-,0,0,0,0,0,-,-' -morphology Thicken '7>:-,-,1,1,1,1,- -,0,1,1,1,1,- -,0,0,1,1,1,- -,0,0,0,1,1,- -,0,0,0,1,1,- -,0,0,0,1,1,- -,0,0,0,-,1,-' -morphology Thicken '7>:-,1,1,1,1,-,- -,1,1,1,1,0,- -,1,1,1,0,0,- -,1,1,0,0,0,- -,1,1,0,0,0,- -,1,1,0,0,0,- -,1,-,0,0,0,-' ter-x24nx4.png


Testing
=======
Rename if need and place nef fonts together with original Terminus `.pcf(.gz)` suite, like at `/usr/share/fonts/BDF/`. Then

    fc-cache -fv

Now we should see our new fonts in browser, desktop and other places capable to display original *Terminus* correctly.


Code
====
We use a large set of standard tools. But we need two extra tools, which are not exist.

* `bdf2image`, i've made it from `bdf2bmp` licensed as LICENSE: BSD style.
* `bdfgrow`, i create it from `bdf2x` licensed as UNLICENSE http://unlicense.org/ . Name changed due to it now support not only 2x upscale, and note that more than 2x zooms are piped input only.


TODO
====
* Bold fonts.
* Create & share complete Terminus suite up to 4x upscaled.
* Read feedback and improve our magic.


LICENSE
-------
This research text description is licensed under Creative Commons Attribution 4.0. You are welcome to contribute to the description in order to improve it so long as your contributions are made available under this same license.

Included software is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.






