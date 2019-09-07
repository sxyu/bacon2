"""
Bacon 2: Hog contest engine.
Python functions and C++ extension functions (_bacon)
are merged into one module.

> import bacon

Usage: Sessions
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

sess.add('id'[, 'name'])  add a strategy with id
sess.remove('id')         remove a strategy with id
sess.clear()              clear all strategies
sess.unlink()             'unlinks' a session, deleting persistence
                          files and converting to transient session
                          which will be deleted when you exit Python

sess.ids()                list all strategy ids
sess.names()              list all strategy names
len(sess)                 number of strategies

Usage: Strategies
sess.strategies()         get a list of Strategy objects
strat = sess['id']        get Strategy object by id
strat[our, opponent]      get roll number in strategy
strat[our, opponent] = x  set roll number in strategy
strat.set_const(i)        set all rolls to a constant
strat.set_random()        set all rolls randomly
strat.array()             get numpy array of roll numbers (int8)
strat.set_array()         set roll numbers from numpy array (must be int8)
strat.draw()              draw a strategy diagram identical to that in
                          the original Bacon

Usage: Contest flow / Results
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
res.array()               get numpy array of win rates

Extra utils
* To render the HTML leaderboard (pretty hacky)
bacon.html.render(res, 'hog.template.html', 'output.html')

* To render the HTML leaderboard (pretty hacky)

* To recursively converts all hog_contest.py in some directories
* returining a list of Strategy objects:
bacon.io.convert(paths...)

* Minor IO utils
bacon.io.write_py(Strategy, path)      create valid hog_contest.py script
                                       returning roll numbers for the
                                       strategy
bacon.io.write_legacy(Strategy, path)  write legacy Bacon .strat format
strat = bacon.io.read_legacy(path)     read legacy Bacon .strat format

* OK integration
oauth = bacon.ok.OKServerOAuthSession()
oauth.authenticate() # returns token, refreshes automatically

To download submissions (authenticates automatically):
oauth.download_assignment_submissions("cal/cs61a/fa18/proj01contest", "hctest", "hog_contest.py")
"""

from _bacon import *
from . import html, io, ok
