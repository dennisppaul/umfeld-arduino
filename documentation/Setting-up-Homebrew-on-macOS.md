# Setting up Homebrew on macOS

on some *clean* homebrew installations on *macOS* the environment variable `$LIBRARY_PATH` is not set or at least does not include the homebrew libraries. if so you may need to add the line `export LIBRARY_PATH=$LIBRARY_PATH:/usr/local/lib` ( for intel-based machines ) or `export LIBRARY_PATH=$LIBRARY_PATH:/opt/homebrew/lib` ( *apple-silicon*-based machines ) to your profile e.g in `~/.zshrc` in *zsh* shell. note, that other shell environments use other profile files and mechanisms e.g *bash* uses `~/.bashrc`. find out which shell you are using by typing `echo $0`.

if you have NO idea what this all means, either run the *Quickstart* installer script or you might just try running the following lines for *zsh* in the terminal:

#### On Intel-based Machines

```sh
{ echo -e "\n# set library path\n"; [ -n "$LIBRARY_PATH" ] && echo "export LIBRARY_PATH=/usr/local/lib:\"\$LIBRARY_PATH\"" || echo "export LIBRARY_PATH=/usr/local/lib"; } >> "$HOME/.zshrc"
source "$HOME/.zshrc"
```

#### On *Apple-Silicon*-based Machines

```sh
{ echo -e "\n# set library path\n"; [ -n "$LIBRARY_PATH" ] && echo "export LIBRARY_PATH=/opt/homebrew/lib:\"\$LIBRARY_PATH\"" || echo "export LIBRARY_PATH=/opt/homebrew/lib"; } >> "$HOME/.zshrc"
source "$HOME/.zshrc"
```

this will permanently set the `LIBRARY_PATH` environement variable in your *zsh* profile file.
