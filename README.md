# Bacon 2: Hog contest engine.

Note: Python functions and C++ extension functions (_bacon)
are merged into one module.

# Installation

## Dependencies

- Python 3. I am using 3.5, should probably work for other minor versions as well.
  - Python 2 seems to work as well, at least for the core functionality (sessions/strategies), but I do not guarentee this

- A modern enough C++ compiler supporting at least C++11
  - These should be available with the OS typically
  - On Windows I recommend using MSYS2, or you can use Visual Studio
  - You may also need to install `libpython3`, `libpython3-dev`, I am not sure

## Steps

```sh
git clone https://github.com/sxyu/bacon2
cd bacon2
pip3 install .
```

## CMake and Running Tests

To use CMake, do as usual:
```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```
By default test building is enabled. To run the tests, use: `./bacon_test` inside the build directory.

### Install with CMake

CMake configuration to setup the Python package is included but not required.

1. Rename `setup.py` to something else, then rename `setup.cmake.py` to `setup.py`

2. `pip3 install .`

# Quickstart example

First time:
```py
import bacon
sess = bacon.Session('fall_20XX')
oauth = bacon.ok.OAuthSession(session = sess)

# will open browser to authorize the first time
oauth.sync(assignment='cal/cs61a/faXX/proj01contest')

sess.run()
bacon.html.render(session=sess, template_path='hog.template.html', output_path='index.html')
# Have some script copy index.html to leaderboard server
```

Later, after reopening Python, everything is saved, and some of the options may be omitted:
```py
import bacon
sess = bacon.Session('fall_20XX')
oauth = bacon.ok.OAuthSession(session = sess)

# should auto-refresh the token unless it has been > 8 hours
oauth.sync()

sess.run()
bacon.html.render(session=sess)
```
Bacon will try to reused cached results and skip converting strategies whenever possible. 

# Detailed Usage

Launch Python 3 and `import bacon`

## Sessions

```
sess = bacon.Session()    create a transient session
                          This type of session will be
                          deleted when you exit Python.
sess = bacon.Session('x') create a persistent session.
                          This session is automatically saved and
                          if you use the same session name after
                          reopening Python, everything including
                          strategies and results will continue
                          to exist.
bacon.sessions()          list all persistent session names.
sess.add('id'[,'name'])   add a strategy with id and display name
                          (display name is shown on scoreboard,
                           id is usually student's email)
                          if name is not specified then the id is used
                          as the name.
sess.add('id', 'name', x) (overflow from above) add a strategy
                          with id and display name
                          and initialize all rolls to x (default
                          is 0)
sess.add_random()         add a strategy with each roll number
                          chosen uniformly at random
sess.add(Strategy)        add a strategy object directly
sess.remove('id')         remove a strategy with id
sess.remove(Strategy)     remove a strategy object
sess.clear()              clear all strategies
sess.clear_results()      clear results (cache), forcing total re-evaluation next time run() is called
sess.unlink()             'unlinks' a session, deleting persistence
                          files and converting to transient session
                          which will be deleted when you exit Python
sess.ids()                list all strategy ids
sess.names()              list all strategy names
len(sess)                 number of strategies
```

## Strategies

```
Strategy(id[, name])      create an empty strategy with id and name
sess.strategies()         get a list of Strategy objects in session
strat = sess['id']        get Strategy object by id from session
xgfzg = sess.by_name('x') get Strategy object by name from session;
                          if multiple strategies with name exist,
                          returns one with lexographically least id.
                          Very inefficient compared to get by id.
strat[our, opponent]      get roll number in strategy
strat[our, opponent] = x  set roll number in strategy
strat.set_const(i)        set all rolls to a constant
strat.set_random()        set all rolls randomly
strat.array()             get numpy array of roll numbers (int8)
strat.set_array()         set roll numbers from numpy array (must be int8)
strat.name                get/set strategy name
strat.id                  get strategy id (immutable)
strat.win_rate(oppo)      alt method to compute win rates
strat.draw()              draw a strategy diagram identical to that in
                          the original Bacon
strat.win_rate_by_sampling(oppo[, num_samples = 10000])
                          compute win rate by sampling num_samples*2 games
                          (num_samples as player0/1 each)
```

## Contest flow / Results

```
sess.run([n_threads])     runs contest (tries to reuse old results).
                          By default uses n_threads=# cores.
                          Returns a Results object (which if not
                          stored has a repr that will print
                          out the rankings table).
res = sess.results()      get the latest results objects. If you
                          have not ran the contest yet, errors.
sess.has_results()        checks whether you have results.
res                       prints contest rankings incl. name, wins
str(res)                  gives more concise version of above
res.rankings              returns a rankings list, which is a list
                          of tuples: (strategy index in contest, wins)
res.ids()                 get list of strategy ids in contest
res.names()               get list of strategy names in contest
res[i]                    get the internal 'half-row' of win rates
                          for player i (representing the triangular
                          matrix) this row will have i entries and
                          allows you to use res[i][j] for i > j
res[i, j]                 get win rate of ith player in contest
                          playing against jth player (integers,
                          order is same as in res.list())
res.array()               get numpy array of win rates:
                          arr[first_player, second_player]
```

## Session Config

To access session config:
```
sess.config()
```
This mostly behaves like a dict but only takes string keys and values. It is serialized with the rest of the session's state. This is used to store data about e.g. OK.

## bacon.config: Constants from Build

The `bacon.config` module contains game constants and functions from `config.hpp` and `config.cpp`. These are immutable in Python.

```
bacon.config.DICE_SIDES         sides of dice, usally 6
bacon.config.GOAL               goal score, usually 100
bacon.config.MIN_ROLLS          min number of dice rolls in a turn
bacon.config.MAX_ROLLS          max number of dice rolls in a turn
bacon.config.MOD_TROT           time trot modulus
bacon.config.FERAL_HOGS_ABSDIFF feral hogs absolute difference
bacon.config.ENABLE_TIME_TROT   whether time trot is enabled
bacon.config.ENABLE_SWINE_SWAP  whether swine swap is enabled
bacon.config.ENABLE_FERAL_HOGS  whether feral hogs is enabled
bacon.config.WIN_EPSILON        min win rate rel. to 0.5 to consider a matchup a win 
bacon.config.swine_swap(a, b)  checks if two scores should result in
                               a swine swap
bacon.config.free_bacon(a)     gets score obtainable through free bacon
```

## Extra utils

* To render the HTML leaderboard (pretty hacky)

```
bacon.html.render(res, 'hog.template.html', 'output.html')
```
OR
```
bacon.html.render(session=sess, template_path='hog.template.html', output_path='output.html')
```
The second form allows you to use just 
```
bacon.html.render(session=sess)
```
after the first call.

* To recursively converts all hog_contest.py in some directories returning a list of Strategy objects:
```
bacon.io.convert(paths...)
```
(kwargs: source_name_suffix: only converts file names with given suffix, verbose: whether to print verbosely)

* To sync a directory, i.e. convert only changed files (previously converted files are saved in the session config and compared)
```
bacon.io.sync_dir(session, dir_path[, source_name_suffix[, verbose]])
```

* Minor IO utils
```
bacon.io.write_py(Strategy, path)      create valid hog_contest.py script
                                       returning roll numbers for the
                                       strategy (similar to hogmat)
bacon.io.write_legacy(Strategy, path)  write legacy Bacon .strat format
strat = bacon.io.read_legacy(path)     read legacy Bacon .strat format
```

* OK integration
```py
oauth = bacon.ok.OAuthSession() # temporary session
oauth = bacon.ok.OAuthSession(session = bacon_session) # persists with Session
oauth.auth() # returns token, refreshes automatically
```
To download submissions:
```
oauth.download("hctest", "cal/cs61a/fa18/proj01contest"[, "hog_contest.py"])
```
(Arguments are: output directory, assignment name, name of file to download from the assignment, default "hog_contest.py")

To sync submissions directly from OK to session in one go (must create session in the second way: bacon.ok.OAuthSession(session)):
```
oauth.sync(assignment = "cal/cs61a/fa18/proj01contest")
```
`assignment` is optional after the first sync.

