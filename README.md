## About

Text editor written from scratch using the [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/) tutorial. Wanted to make it my own (in fact, this was written with `marrow`!), so I've added/am adding some extra, opinionated features:

* Configuration file in `config.h`, with options for features like line numbers, Git gutters, and WakaTime integration
* Syntax highlighting for other programming languages besides C. The way the guide does syntax highlighting is quite frankly horrendous, but I can't think of a quicker way to do it without generating tokens for each language to categorize each term. I'll start by modularizing the syntax highlighting code and then adding a couple of languages that I use often, like Python and JavaScript.
* A tree viewer. This should be relatively easy using `dirent.h` (not supported in Windows though) and adapting the code currently being used to track keystrokes to track moving through the file tree.
* Tab system. I already use iTerm so I suppose I could just use `Command` + `number` to open a new tab, but I think it would be cool to have a tab system directly in Marrow where I can switch between editor views to edit different files. Split view can come later, if I'm so inclined.
* Terminal emulator? Not sure if I'm using the correct term. Vim uses `libvterm`, so I want to do a little more research into that.

These are all features I use in Vim/VSCode on a daily basis and thought would be useful to add on.

## Compatibility

Want to try this out? Great. I've only run this on my Macbook, so not sure if it works on Linux. On Windows, I am definitely sure you can't run it, although I'm not sure if you might need to use something like Cygwin because some of the libraries (`unistd.h` on the top of my head) most likely don't work directly on Windows.

## Wait, so are you actually using this?

Yes! Just mentioned it before, but I wrote this README with `marrow`. I simply set up an alias for `zsh` (I'm on a Macbook at the time of writing) so I can run it with `marrow <file>`.

Does this mean I'm using it solely by itself? Nope. This was intended to replace Vim in my Vim + VSCode setup, although I think I'll still be using all three. (This for funsies, Vim to learn Vim, and VSCode when I need to switch between multiple files every few seconds). It's a lot of fun using something you built!
 

