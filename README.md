# esp32_cw_keyer

This is my morse code keyer for the Espressif ESP32 platform microcontrollers.

It's a work in progress.

Currently I use an M5 Core as my "master" keyer that has inputs for the paddles. I paddle into the master, it gives me sidetone and on-screen display of what I've keyed. As each character is resolved, the master sends that character over wifi to a "slave" keyer which keys my transmitter. You don't have to do this, you could use it more traditionally.

There's a web page running on the esp32 (both master and slave) that allow for configuring things like wpm, farnsworth spacing, etc. Also you can type a message in the page and the keyer will then key it.

This started life as a fork of https://github.com/k3ng/k3ng_cw_keyer

But since then, although I still use some of K3NG's code, the projects are now very, very different. At first I just wanted to get his code to work on ESP32, which I did. How are they different?

a) I only care about supporting esp32. I'm a microcontroller hobbyist and prefer the ESP32 over Arduino. I think it's a better value.
a1) In fact, my keying is based on hardware and interrupt timers that are quite specific to the esp32 (though could be abstracted to other platforms, I'm sure). K3NG relies on more traditional millis()-checks but this means you really have to know what you're doing if you touch things because the timing is all very sensitive. With interrupts, there's less concern but they do create their own challenges.

b) I don't care about firmware file size. K3NG makes a lot of effort to allow for compilation on even an Arduino nano by an elaborate #define mechanism to limit the feature set. The ESP32 has so much headroom that there's little (at this time) to be gained for me to be concerned with that. This means I can just configure whatever I want at run-time through a web page, rather than at compile time.

c) I'm very naive about morse and keying, so my worldview on feature set is very narrow. K3NG has many features, but right now I don't care about them. When I do, I'll add them to this.

d) I use platformio in vscode, abandoned the arduino ide a long time ago!

e) I wanted to migrate keyer functionality, i.e. the keyer domain model, to an OOP approach. This way I can easily swap in new hardware or other software libraries or ideas without having to touch too many lines of code or break other functionality.

f) No straight-key. I expect dit and dah on separate pins.

g) No fancy iambic squeeze stuff. Basically my concept is that all paddles are "rocker style" like e.g. Bencher ST-style, where the paddles are either pressing a dit or dah but not both. So for example, if you have iambic paddle and press a dit, then press a dah while still holding the dit, my approach is to assume you meant to release the dit before you pressed dah.

In my day job I use C#, so my C++ is rusty and probably reflects the mindset of a C#-developer. Oh well.

Who is this for?

If you are new to CW and for whatever reason decided you would rather build than buy a keyer (or just use your radio's) there are other projects and communities that are better suited, e.g. K3NG and the radioartisan groups.io forum. That's how I started, by getting that built on an Arduino Nano and used that for my first keyer.

There are probably better and more sophisticated esp32 keyer options out there too, maybe even open source. But frankly I don't care because I enjoy using this as a learning platform to solve my own problems and have complete control over the keyer.
