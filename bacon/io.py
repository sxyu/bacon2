"""
IO methods for Bacon 2.
Includes Hogconv Python strategy converter
directly adapted from Hogconv.py in original Bacon.
Very messy, handles many edge cases I encountered in production.
"""

def write_legacy(strat, path):
    """ Write given strategy to legacy .strat file """
    from _bacon import config as _bacon_config

    fout = open(path, 'w', encoding='UTF-8')
    fout.write("strategy " + strat.id + "\n")
    for r in range(_bacon_config.GOAL):
        for c in range(_bacon_config.GOAL):
            if c:
                fout.write(' ')
            fout.write(str(strat[r, c]))
        fout.write('\n')
    fout.close()


def read_legacy(path):
    """ Read strategy from strat file """
    from _bacon import Strategy as _Strategy, config as _bacon_config
    fin = open(path, 'r', encoding='UTF-8')
    lines = fin.readlines()
    strat_id = ' '.join(lines[0].split()[1:])
    strat = _Strategy(strat_id)
    for r in range(_bacon_config.GOAL):
        for c, x in enumerate(map(int, lines[r+1].split())):
            strat[r, c] = x
    return strat


def write_py(strat, path):
    """ Re-create hogmat.py-style hog_contest.py file from given strategy """
    from _bacon import config as _bacon_config

    fout = open(path, 'w', encoding='UTF-8')
    fout.write("PLAYER_NAME = '{}'\ndef final_strategy(score, opponent_score):\n    return [".format(strat.id))
    for r in range(_bacon_config.GOAL):
        if r:
            fout.write('\n')
        fout.write('[')
        for c in range(_bacon_config.GOAL):
            if c:
                fout.write(', ')
            fout.write(str(strat[r, c]))
        fout.write(']')
    fout.write("][score][opponent_score]\n")
    fout.close()


# Configurations
SOURCE_NAME_SUFFIX = 'hog_contest.py' # source file must end in this
STRATEGY_FUNC_ATTR = 'final_strategy' # attribute of student's module that contains the strategy function
TEAM_NAME_ATTRS = ['PLAYER_NAME', 'TEAM_NAME'] # allowed attributes of student's module that contains the team name

TEAM_NAME_MAX_LEN = 100 # max length for team names (set to 0 to remove limit)
DEF_EMPTY_TEAM_NAME = "<no name given, email starts with {}>" # name for teams with an empty team name

ERROR_DEFAULT_ROLL = 5 # default roll in case of invalid strategy function (will report error)

TIMEOUT_SECONDS = 45 # max time a student's submission should run

def convert(*args, **kwargs):
    """ Magic function to convert a Python file, usually hog_contest.py """
    from functools import wraps
    from _bacon import Strategy as _Strategy, config as _bacon_config
    import os, sys, imp, random, string, re, errno, threading, time, queue
    GOAL = _bacon_config.GOAL
    MIN_ROLLS, MAX_ROLLS = _bacon_config.MIN_ROLLS, _bacon_config.MAX_ROLLS

    count = 0

    # dict of names, used to check for duplicate team names
    output_names = {}
    outputs = []
    all_errors = []
    verbose = True

    # print to stderr
    def eprint(*args, **kwargs):
        """ print to stderr """
        if verbose:
            print(*args, file=sys.stderr, **kwargs)

    class Worker(threading.Thread):
        def __init__ (self, func, args, kwargs, q):
            threading.Thread.__init__ (self)
            self.func = func
            self.args = args
            self.kwargs = kwargs
            self.q = q
            self.setDaemon (True)
        def run (self):
            self.q.put((True, self.func(*self.args, **self.kwargs)))

    class Timer(threading.Thread):
        def __init__ (self, timeout, error_message, worker, q):
            threading.Thread.__init__ (self)
            self.timeout = timeout
            self.error_message = error_message
            self.worker = worker
            self.q = q
            self.setDaemon (True)
        def run (self):
            time.sleep(self.timeout)
            self.q.put((False, None))

    def timeout(seconds=10, error_message=os.strerror(errno.ETIME)):
        """ makes function error if ran for too long """
        def decorator(func):
            def wrapper(*args, **kwargs):
                q = queue.Queue()
                worker = Worker(func, args, kwargs, q)
                timer = Timer(seconds, error_message, worker, q)
                worker.start()
                timer.start()
                code, result = q.get()
                if worker.isAlive():
                    del worker
                if timer.isAlive():
                    del timer
                if code:
                    return result
                else:
                    eprint("ERROR: Conversion timed out (> {} s) for: ".format(TIMEOUT_SECONDS) + args[0])

            return wraps(func)(wrapper)

        return decorator

    @timeout(TIMEOUT_SECONDS)
    def convert(file):  
        """ convert a single file """
        import shutil, os, sys
        sys.setrecursionlimit(200000)
        module_path = file[:-3] # cut off .py

        module_dir, module_name = os.path.split(module_path)

        # make sure module's dependencies work
        sys.path[-1] = module_dir
        sys.path.append(os.path.dirname(os.path.realpath(__file__)))

        module_dir_name = ""
        if '/' in module_dir:
            module_dir_name = module_dir[module_dir.rindex('/')+1:]
        elif '\\' in module_dir:
            module_dir_name = module_dir[module_dir.rindex('\\')+1:]
        if not module_dir_name:
            module_dir_name = module_name

        # import module
        try:
            from . import dice, hog, ucb
            module = imp.load_source('hog_contest', file)
            # try to prevent use of dangerous libraries, although not going to be possible really
            module.subprocess = module.shutil = "trolled"
            if hasattr(module, "os"):
                module.os.rmdir = module.os.remove = "trolled"

        except Exception as e:
            # report errors while importing
            eprint("\nERROR: error occurred while loading " + file + ":")
            eprint(type(e).__name__ + ': ' + str(e))
            all_errors.append((file, "[Skipped] Import error: " + type(e).__name__ + " (maybe you are trying to import a module e.g. hog, dice, numpy?)", "with email: " + module_dir_name[:2] + "...@"))
            eprint("skipping...\n")
            return
        if hasattr(module, STRATEGY_FUNC_ATTR):
            strat = getattr(module, STRATEGY_FUNC_ATTR)
        else:
            all_errors.append((file, "[Skipped] Missing " + STRATEGY_FUNC_ATTR))
            eprint("ERROR: " + file + " has no attribute " + STRATEGY_FUNC_ATTR + " , skipping...")
            del module
            return

        output_name = ""
        for attr in TEAM_NAME_ATTRS:
            if hasattr(module, attr):
                val = str(getattr(module, attr))
                if val:
                    output_name = getattr(module, attr)
                setattr(module, attr, "")

        if not output_name:
            eprint("WARNING: submission " + file + " has no team name. Using default name...")
            all_errors.append((file, "[Warning] Team name is empty or does not exist", "with email: " + module_dir_name[:2] + "...@"))
            output_name = DEF_EMPTY_TEAM_NAME.format(module_dir_name[0])

        # check for team names that are too long
        if len(output_name) > TEAM_NAME_MAX_LEN and TEAM_NAME_MAX_LEN > 0:
            eprint("WARNING (minor): " + file + " has a team name longer than " + str(TEAM_NAME_MAX_LEN) + 
                    " chars. Truncating...")
            output_name = output_name[:TEAM_NAME_MAX_LEN-3] + "..."

        # check for duplicate team names
        strat_name = re.sub(r"[\r\n]", "", output_name)
        try:
            output_name = output_name.encode('ascii','ignore').decode('ascii')
            output_name = re.sub(r"[\\/:*?""<>|+=,\r\n]", "", output_name)
        except:
            output_name = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(12))
        if output_name in output_names:
            strat_name += "_" + str(output_names[output_name])
            output_names[output_name] += 1
            all_errors.append((file, "[Warning] Duplicate team name " + output_name, strat_name))
            eprint("WARNING: found multiple teams with name",
                    output_name)
        else:
            output_names[output_name] = 1

        # construct new strategy
        out_strat = _Strategy(module_dir_name, strat_name)
        nerror = 0
        errname = ""

        for i in range(GOAL):
            for j in range(GOAL):
                try:
                    rolls = strat(i, j)

                    # check if output valid
                    if type(rolls) != int or rolls < MIN_ROLLS or rolls > MAX_ROLLS:
                        if type(rolls) != int:
                            errname = "WARNING: team " + strat_name + "'s strategy function outputted something other than a number!"
                        else:
                            errname = "WARNING: team " + strat_name + "'s strategy function outputted an invalid number of rolls:" + str(rolls) + "!"
                        nerror+=1
                        rolls = ERROR_DEFAULT_ROLL

                    out_strat[i, j] = rolls
                except Exception as e:
                    # report errors while running strategy
                    nerror += 1
                    errname = type(e).__name__ + " " + str(e)
                    out_strat[i, j] = ERROR_DEFAULT_ROLL

        if nerror:
            eprint("\nERROR: " + str(nerror) + " error(s) occurred while running " + STRATEGY_FUNC_ATTR + ' for ' + strat_name + '(' + file + "):") 
            all_errors.append((file, "[Partial failure] " + errname, strat_name))
            eprint(errname)

        outputs.append(out_strat)
        eprint("hogconv: converted strategy " + module_dir_name + " name='" + strat_name + "'")

        del module
        nonlocal count
        count += 1 # counting how many converted

    def convert_dir(dir_path = os.path.dirname(__file__)):
        """ convert all files in a directory (does not recurse) """
        for file in os.listdir(dir_path or None):
            path = os.path.join(dir_path, file)
            if os.path.isdir(path):
                convert_dir(path)
            elif file == '__init__.py' or file == __file__ or \
                 file[-len(SOURCE_NAME_SUFFIX):] != SOURCE_NAME_SUFFIX:
                continue
            else:
                convert(path)

    # add current directory to sys.path to load any custom depencencies
    sys.path.append('')

    for i in range(len(args)):
        path = args[i]
        if os.path.exists(path):
            if os.path.isdir(path):
                convert_dir(path)
            else:
                convert(path)
        else:
            eprint("ERROR: can't access " + path + ", skipping...")

    verbose = not ('verbose' in kwargs and not kwargs['verbose'])

    if len(args) < 1:
        eprint("Expected at least 1 argument: file name.")  
        return;
    else:
        eprint("converted a total of " + str(count) + (" strategies." if count != 1 else " strategy."))

    if all_errors:
        print("ERRORS occurred during conversion:")
        for path, error_msg, identifier in all_errors:
            print("Team", identifier, ": ", error_msg, ". At path", path)
    return outputs

