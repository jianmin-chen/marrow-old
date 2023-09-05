## TODO

Oops this project just keeps getting bigger and bigger

- [ ] Fix "inconsistent use of spaces and tabs" according to Python - 1 day
- [ ] `u` and `r` keystroke - 3 days
- [ ] Soft wrap choice - 1 day
- [ ] Support highlighting ticks in JavaScript - 1 day
- [ ] Load config into memory instead of directly working with it + add support for updating config - 1/2 day
- [ ] Get tree view similar to Vim's ":Ex" working, starting in the directory of the given file or just the default directory if one isn't provided - 1 week
- [ ] Be able to move around and open files through the tree view - 1 week
- [ ] Get tab system working, `;` and Ctrl+number to move between tabs - 1 week
- [ ] Deal with file being edited outside of Marrow - 2 days
- [ ] Highlight other closing symbol - 2 days
- [ ] Add background color & extra terminal coloring to Marrow (test with favorite themes: gruvbox, monokai, synthwave, github) - 2 days
- [ ] Flag for underscore to separate large numbers semantically - 1/2 day
- [ ] Autocomplete system - 1 week
- [ ] Extensible plugin system with Lua - 1 month
- [ ] Test plugin system by rewriting code for line numbers and also Git gutters - 1 week

= 2 months = end of October. Welcome to patience!

## About

Text editor written from scratch based on the [Build](https://viewsourcecode.org/snaptoken/kilo/) Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/) tutorial that I daily dogfood. Wanted to make it my own (in fact, this was written with `marrow`!), so I've added/am adding ~~some~~ a lot of extra, opinionated features. Libraries currently being used include:

* [`inih`](https://github.com/benhoyt/inih), for parsing INI files
* [`tiny-regex-c`](https://github.com/kokke/tiny-regex-c), for parsing regex strings

Although I would love to write my own in the future.

## Design decisions

Over the next couple of months, I'll be working on adding the following, which are obviously all quite opinionated:

- [X] Different modes like Vim.
- [X] Configuration file in `config.h`, with options for features like line numbers, Git gutters, and WakaTime integration (all things I use in my Vim config) 
- [X] Syntax highlighting for other programming languages besides C. The way the guide does syntax highlighting is quite frankly horrendous, but I can't think of a quicker way to do it without generating tokens for each language to categorize each term. I'll start by modularizing the syntax highlighting code and then adding a couple of languages that I use often, like Python and JavaScript.
- [X] A tree viewer. This should be relatively easy using `dirent.h` (not supported in Windows though) and adapting the code currently being used to track keystrokes to track moving through the file tree.
- [X] Tab system. Split view can come later if I'm so inclined.
- [X] Terminal emulator? Wrapper around `libvterm`, except I don't know how to get started. (Ended up writing my own, which isn't the best in terms of functionality but I don't understand exactly how to use `libvterm` yet.)
- [X] Autocomplete system.
- [X] Figure out a better plugin system so this is actually extensible (and usable by other people).
- [ ] Optimizing the code. Ran it through Valgrind so there aren't any memory issues, but some parts of the code could definitely be written better.
- [ ] Potentially writing all the libraries I use from scratch (I know, I know).

These are all features I use in Vim/VSCode on a daily basis and thought would be useful to add on. 

## Compatibility

Want to try this out? Great. I've only run this on my Macbook and Ubuntu on an EC2 instance. On Windows, I'm pretty sure this can't run, although I might patch issues in the future if I ever need to work on Windows.

## Wait, so are you actually using this?

Yes! Just mentioned it before, but I wrote this README with `marrow`. I simply moved the executable into `/usr/local/bin` and it works like a charm.

Does this mean I'm using it solely by itself? Nope. This was intended to replace Vim in my Vim + VSCode setup, although I think I'll still be using all three. (This for funsies, Vim to learn Vim, and VSCode when I need to switch between multiple files every few seconds). It's a lot of fun using something you built!