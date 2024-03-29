"""
Bacon OK integration: mostly ported from OK Client
https://github.com/okpy/ok-client/blob/master/client/utils/auth.py
"""
import sys, requests, logging, webbrowser, http.server, time
from urllib.parse import urlencode, urlparse, parse_qsl

log = logging.getLogger(__name__)

CLIENT_ID = 'hog-contest'

# Don't worry, it is ok to share this
CLIENT_SECRET = '1hWDyQjZS6PSVVhAJzQpUqzkYPyM0EN'

OAUTH_SCOPE = 'all'

# Localhost
REDIRECT_HOST = "127.0.0.1"
REDIRECT_PORT = 6265

# OAuth post timeout 
TIMEOUT = 10

# Server/API config
OK_SERVER_URL = 'https://okpy.org' 
INFO_ENDPOINT = '/api/v3/user/'
ASSIGNMENT_ENDPOINT = '/api/v3/assignment/'
AUTH_ENDPOINT =  '/oauth/authorize'
TOKEN_ENDPOINT = '/oauth/token'
ERROR_ENDPOINT = '/oauth/errors'

# URL to redirect user to upon OAuth success
SUCCESS_ENDPOINT_URL = 'https://okpy.org' # temporary

class BaconOkException(Exception):
    """Base exception class for Bacon/OK integration."""
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        log.debug('Exception raised: {}'.format(type(self).__name__))
        log.debug('python version: {}'.format(sys.version_info))
        log.debug('okpy version: {}'.format(client.__version__))

class OAuthException(BaconOkException):
    """ OAuth related exception """
    def __init__(self, error='', error_description=''):
        super().__init__()
        self.error = error
        self.error_description = error_description

def _pick_free_port(hostname=REDIRECT_HOST, port=0):
    """ Try to bind a port. Default=0 selects a free port. """
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.bind((hostname, port))  # port=0 finds an open port
    except OSError as e:
        if port == 0:
            print('Unable to find an open port for authentication.')
            raise BaconOkException(e)
        else:
            return _pick_free_port(hostname, 0)
    addr, port = s.getsockname()
    s.close()
    return port

def _make_token_post(server, data):
    """Try getting an access token from the server. If successful, returns the
    JSON response. If unsuccessful, raises an OAuthException.
    """
    try:
        response = requests.post(server + TOKEN_ENDPOINT, data=data, timeout=TIMEOUT)
        body = response.json()
    except Exception as e:
        log.warning('Other error when exchanging code', exc_info=True)
        raise OAuthException(
            error='Authentication Failed',
            error_description=str(e))
    if 'error' in body:
        log.error(body)
        raise OAuthException(
            error=body.get('error', 'Unknown Error'),
            error_description = body.get('error_description', ''))
    return body

def _make_code_post(server, code, redirect_uri):
    data = {
        'client_id': CLIENT_ID,
        'client_secret': CLIENT_SECRET,
        'code': code,
        'grant_type': 'authorization_code',
        'redirect_uri': redirect_uri,
    }
    info = _make_token_post(server, data)
    return info['access_token'], int(info['expires_in']), info['refresh_token']

def _make_refresh_post(server, refresh_token):
    data = {
        'client_id': CLIENT_ID,
        'client_secret': CLIENT_SECRET,
        'grant_type': 'refresh_token',
        'refresh_token': refresh_token,
    }
    info = _make_token_post(server, data)
    return info['access_token'], int(info['expires_in']), info['refresh_token']

def _get_code():
    """ Make the requests to get OK access code """
    #email = input("Please enter your bCourses email: ")

    host_name = REDIRECT_HOST
    try:
        port_number = _pick_free_port(port=REDIRECT_PORT)
    except BaconOkException:
        # Could not bind to REDIRECT_HOST:0, try localhost instead
        host_name = 'localhost'
        port_number = _pick_free_port(host_name, 0)

    redirect_uri = "http://{0}:{1}/".format(host_name, port_number)

    params = {
        'client_id': CLIENT_ID,
        'redirect_uri': redirect_uri,
        'response_type': 'code',
        'scope': OAUTH_SCOPE,
    }
    url = '{}{}?{}'.format(OK_SERVER_URL, AUTH_ENDPOINT, urlencode(params))
    try:
        assert webbrowser.open_new(url)
        return _get_code_via_browser(redirect_uri,
            host_name, port_number)
    except Exception as e:
        import traceback
        log.debug('Error with Browser Auth:\n{}'.format(traceback.format_exc()))
        log.warning('Browser auth failed, falling back to browserless auth')
        # Try to do without a browser
        redirect_uri = 'urn:ietf:wg:oauth:2.0:oob'
        print("\nOpen this URL in a browser window:\n")
        print("{}/oauth/authorize?response_type=code&redirect_uri=urn%3Aietf%3Awg%3Aoauth%3A2.0%3Aoob&client_id={}&scope={}".format(OK_SERVER_URL, CLIENT_ID, OAUTH_SCOPE))
        code = input('\nPaste your code here: ')
        return _make_code_post(OK_SERVER_URL, code, redirect_uri)

def _get_code_via_browser(redirect_uri, host_name, port_number):
    """ Get OK access code by opening User's browser """
    server = OK_SERVER_URL
    code_response = None
    oauth_exception = None

    class CodeHandler(http.server.BaseHTTPRequestHandler):
        def send_redirect(self, location):
            self.send_response(302)
            self.send_header("Location", location)
            self.end_headers()

        def send_failure(self, oauth_exception):
            params = {
                'error': oauth_exception.error,
                'error_description': oauth_exception.error_description,
            }
            url = '{}{}?{}'.format(server, ERROR_ENDPOINT, urlencode(params))
            self.send_redirect(url)

        def do_GET(self):
            """Respond to the GET request made by the OAuth"""
            nonlocal code_response, oauth_exception
            log.debug('Received GET request for %s', self.path)
            path = urlparse(self.path)
            qs = {k: v for k, v in parse_qsl(path.query)}
            code = qs.get('code')
            if code:
                try:
                    code_response = _make_code_post(server, code, redirect_uri)
                except OAuthException as e:
                    oauth_exception = e
            else:
                oauth_exception = OAuthException(
                    error=qs.get('error', 'Unknown Error'),
                    error_description = qs.get('error_description', ''))

            if oauth_exception:
                self.send_failure(oauth_exception)
            else:
                self.send_redirect(SUCCESS_ENDPOINT_URL)

        def log_message(self, format, *args):
            return

    server_address = (host_name, port_number)
    log.info("Authentication server running on {}:{}".format(host_name, port_number))

    try:
        httpd = http.server.HTTPServer(server_address, CodeHandler)
        httpd.handle_request()
    except OSError as e:
        log.warning("HTTP Server Err {}".format(server_address), exc_info=True)
        raise

    if oauth_exception:
        raise oauth_exception
    return code_response

class OAuthSession:
    """ Represents OK OAuth state """
    def __init__(self, access_token='', refresh_token='', expires_at=-1, session = None):
        """ Create OK OAuth state with given tokens, and expiration """
        self.session = self.refresh_token = self.access_token = None
        self.expires_at = -1
        self.assignment = None
        if session is not None: 
            config = session.config()
            self.session = session
            if 'ok_access_token' in config:
                self.access_token = config['ok_access_token']
            if 'ok_refresh_token' in config:
                self.refresh_token = config['ok_refresh_token']
            if 'ok_expires_at' in config:
                self.expires_at = int(config['ok_expires_at'])
            if 'ok_last_download_assignment' in config:
                self.assignment = config['ok_last_download_assignment']
        elif access_token and refresh_token and expires_at >= 0:
            self.access_token = str(access_token)
            self.refresh_token = str(refresh_token)
            self.expires_at = expires_at

    def _dump(self):
        """ Dump state to a Bacon session """
        if self.session is not None:
            config = self.session.config()
            if self.access_token:
                config['ok_access_token'] = self.access_token
            if self.refresh_token:
                config['ok_refresh_token'] = self.refresh_token
            if self.expires_at >= 0:
                config['ok_expires_at'] = str(self.expires_at)
            if self.assignment:
                config['ok_last_download_assignment'] = self.assignment

    def refresh(self):
        """ Refreshes a token """
        if not self.refresh_token:
            return False
        cur_time = int(time.time())
        if cur_time < self.expires_at - 3600:
            # expires in 1 hour
            return True
        self.access_token, expires_in, self.refresh_token = _make_refresh_post(OK_SERVER_URL, self.refresh_token)
        if not (self.access_token and expires_in):
            log.warning("Refresh authentication failed and returned an empty token.")
            return False
        cur_time = int(time.time())
        self.expires_at = cur_time + expires_in
        self._dump()
        return True

    def auth(self, force_reauth = False):
        """
        Returns OAuth access token which can be passed to the server
        for identification. If force_reauth is specified then will
        force re-authenticate the user; else tries to reuse or
        refresh previous token
        """
        # Attempts to import SSL or raises an exception
        try:
            import ssl
        except:
            log.warning('Error importing SSL module', stack_info=True)
            print(SSL_ERROR_MESSAGE)
            sys.exit(1)
        else:
            log.info('SSL module is available')

        # Refresh the token if not forcing reauth
        if not force_reauth and self.refresh():
            return self.access_token

        # Perform OAuth
        print("Token is not available, performing OAuth")
        try:
            self.access_token, expires_in, self.refresh_token = _get_code()
        except UnicodeDecodeError as e:
            with format.block('-'):
                print("Authentication error\n:{}".format(HOSTNAME_ERROR_MESSAGE))
        except OAuthException as e:
            with format.block('-'):
                print("Authentication error: {}".format(e.error.replace('_', ' ')))
                if e.error_description:
                    print(e.error_description)
        else:
            cur_time = int(time.time())
            self.expires_at = cur_time + expires_in
            self._dump()
        return self.access_token

    def download(self, output_dir, assignment = '', subm_file_name = 'hog_contest.py', email_separator = '-', quiet = False):
        """
        Download all submissions for an assignment from OK to the specified directory.
        Example:
        oauth.download_assignment_submissions("~/hog_contest_submissions",
                "cal/cs61a/fa18/proj01contest", "hog_contest.py")
        If assignment is not given, will use the last used assignment
        if available, errors else.
        """
        self.auth()
        if not self.access_token:
            raise OAuthException(
                error='Authentication failed, could not download assignment')

        if not assignment:
            assignment = self.assignment
        else:
            self.assignment = assignment
            self._dump()

        ok_subm_api_url = "{}{}{}/submissions?access_token={}".format(OK_SERVER_URL, ASSIGNMENT_ENDPOINT, assignment, self.access_token)

        import json, urllib, os
        from os.path import join
        with urllib.request.urlopen(ok_subm_api_url) as response:
           html = response.read().decode('UTF-8')
           jsonfile = json.loads(html)
           if jsonfile['code'] != 200:
               log.error('OK API HTTP error: ' + str(jsonfile['code']))
               return
        dat = jsonfile['data']

        cnt = 0
        for subm in dat['backups']:
            if not subm['submit']: continue
            out_name = email_separator.join((student['email'] for student in subm['group']))
            for msg in subm['messages']:
                 if subm_file_name in msg['contents']:
                     contents = msg['contents'][subm_file_name]
                     break
            else:
                 continue
            cnt += 1
            subm_dir = join(output_dir, out_name)
            os.makedirs(subm_dir, exist_ok=True)
            #contents = contents.replace('\t', '    ') # HACK to fix tab problems
            with open(join(subm_dir, subm_file_name), 'w', encoding="UTF-8") as f:
                f.write(contents)
        if not quiet:
            print("OAuthSession.download: {} submissions downloaded from OK server".format(cnt))

    def sync(self, tmp_dir_name = 'bacon-hogcontest-tmp', assignment = '', subm_file_name = 'hog_contest.py', quiet = False):
        """ Direct session synchronization """
        if self.session is None:
            log.error('Cannot sync an OAuthSession with no attached bacon.Session')
            return
        if assignment:
            self.assignment = assignment
        if not self.assignment:
            log.error('The assignment argument is not specified and you have not synced this session previously. Please use oauth_session.sync(assignment="assignment_name"), where assignment_name is for example "cal/cs61a/fa18/proj01contest".')
            return

        import os#, sys, imp, random, string, re, errno, threading, time, queue
        os.makedirs(tmp_dir_name, exist_ok=True)

        if not quiet:
            print("OAuthSession.sync: Downloading submissions from OK server...")

        self.download(tmp_dir_name, assignment=assignment, subm_file_name=subm_file_name, quiet=quiet)

        if not quiet:
            print("OAuthSession.sync: Converting and importing strategies...")
        from .io import sync_dir
        sync_dir(self.session, tmp_dir_name, source_name_suffix=subm_file_name, verbose=not quiet)

        import shutil, os
        shutil.rmtree(tmp_dir_name, True) # clean up

