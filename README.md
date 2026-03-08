# terminalcam

Real-time ASCII art webcam renderer for the terminal. Captures live webcam input via OpenCV and renders it as ASCII characters using LUMA-weighted brightness mapping.

```
<<~+__-??][}}}{{1)((||||/////ttttttfffffffffjjjjfffttttttttttt/|||||)1{}[]-+>iili><<~~~++++~
!!i><<~~+_-]][[}{1))(|||/////tttttttfffffffffffffttttttttttt//|(|((1{}[]?_~ii!i<<~~+++++++++
Ill!ii><~~+_-?][}}{1)(||/////tttttttfffffffffffftttt////////|(((()[II<?-iI!~i<~~~~+++++++++}
lll!!i><<~++_-?][}{{1)(|||////ttttttttttttttttt/////////|||((())1{<   :<  .-+++++++<+++~_)jj
!!!!ii><~l>__-?][}{11)((||/////tttt/)_,.  ^I+{|||||||((((())1i:![[]`   >l  '1__+++" "}}txj({
~~~~~+__~  :I;li!>>il>{|/////ttt/},            '<1))))))111{{'  :?_<   ^_^  [-__++  ^fnj/|()
<<~+___-"'^`""":"",;"`<|/ttttt/]                 '-{{{{}}}[[]_   l>>,   :>  :t_+~>  "1]|xj/|
    .`"I'.^" ..^`.    <{|/tttt<    .''```^^:l<?->; +[[[]]]??-_!   ;~+^ .`i  .1__-,  :1[{tfff
          l>i!!ll:"^`>]}1)tff[      .`^^,",;<]1/ff>.]}}[[]]??-_^  .<-+'..I< .,{{1` '~][{(jjr
         ':I><<>>>>+-[-+1tfff-           ,'   '!)xj`-{}}}[[]]??-" `"??i..`,'.`;<-. `+[?1)fxx
         ,;;IIli>iii>?+<)ffff/           !;    ~)xv;+11{{{}[[[]?_..`",^'```^^"""^`",>f]]|/xn
                     ^+itffjjf'          "(_".'~{jv;)(()1i':+}[]?l '^,,"```'`^^^^^",;[/})jzU
                      l{fjjjj).  '``'    ,1j}-~?(jc{-)||((,.'i}[]-..`^,:,^^"^`^"^^",I~cj1ruU
                      ^i(jjjj+   ..'.     ;_~![1/xz+!)//||[ .`!?[]:`^";;I;:,,^^""",:l<fY/(vc
    `^II;  !l,`      ; `lffjjf"    .        `:_[tnct)ttttt/?^`^"I<"";IIll!lI;:,,",:;li1Yc(tu
    "l!>+< [~I       I" 1tffjjt~^           `  "(nvffffffftt/l`,,:"":Illl!!!lI;::::I!>}cUr|n
       .`:I!l;       I"./tffjjjr|'         I})-[/xx'',;?fjjfft! '`^":;Illl!!!llIIIIli~|YUYt/
                    lI.<tffjjrr[;         `!])/ttj|      I>,;>)[i^'`^,::;IlllllllIl!>+(YUUct
',,"""""""^'.       >l 1ffjrrxx           'l+)|)f{r       l    <<~<I:`^"::;;IIIII;;I!<[zUUUx
                   `~! frx)<,               l-1|]{(       .          I,_~i,,,",:;Illli?vYUUY
                   ~i^                    'I+{[~]]`                    :;{r[,,,:;IIll!>{1!{z
-_           `   '                       .',>_-+i'                       "I-[~`,,:;;;II!l` <
                                          .`IiI^                         "   '  .'`^^^`'   '
            .
l":ll       ^
```

## Prerequisites

```
brew install opencv
```

## Build & Run

```
make
./terminalcam
```

On first run, macOS will prompt for camera access -- grant it.

## Controls

| Key   | Action                       |
|-------|------------------------------|
| `q`   | Quit                         |
| `b`   | Decrease brightness          |
| `B`   | Increase brightness          |
| `i`   | Invert (toggle light/dark)   |
| `f`   | Toggle FPS display           |
| `^C`  | Quit                         |

## How It Works

Each frame from the webcam is divided into a grid of rectangular blocks sized to fill the terminal. Each block's average color is converted to brightness using the LUMA formula:

```
brightness = 0.299 * R + 0.587 * G + 0.114 * B
```

The brightness value (0-255) maps to a position in a 70-character ASCII ramp from light to dark:

```
 .'`^",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$
```

The image is flipped horizontally for a mirror effect. Terminal character cells are taller than they are wide (~2.2:1), so block height is scaled accordingly to preserve aspect ratio.

Rendering uses ANSI cursor-home (`\033[H`) instead of screen-clear to eliminate flicker, with the entire frame written in a single `write()` call.
