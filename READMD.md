## About

Text editor written from scratch using the [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/) tutorial. Wanted to make it my own (in fact, this was written with `marrow`!), so I've added/am adding some extra, opinionated features:

* Configuration file in `config.h`, with options for features like line numbers, Git gutters, and WakaTime integration

These are all features I use in Vim/VSCode on a daily basis and thought would be useful to add on.

## Compatibility

Want to try this out? Great. I've only run this on my Macbook, so not sure if it works on Linux. On Windows, I am definitely sure you can't run it, although I'm not sure if you might need to use something like Cygwin because some of the libraries (`unistd.h` on the top of my head) most likely don't work directly on Windows.

## Wait, so are you actually using this?

Yes! Just mentioned it before, but I wrote this README with `marrow`. I simply set up an alias for `zsh` (I'm on a Macbook at the time of writing) so I can run it with `marrow <file>`.

Does this mean I'm using it solely by itself? Nope. This was intended to replace Vim in my Vim + VSCode setup, although I think I'll still be using all three. (This for funsies, Vim to learn Vim, and VSCode when I need to switch between multiple files every few seconds). It's a lot of fun using something you built!
 

