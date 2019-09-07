""" Bacon HTML rendering util """


def render(results = None, template_path = '', output_path = '', session = None):
    """
    Render the HTML leaderboard. Contains very basic
    (hacky) template capabilities.
    """
    if session is not None:
        results = session.results()
        config = session.config()
        if not template_path:
            if 'html_template_path' in config:
                template_path = config['html_template_path']
            else:
                raise RuntimeError("template_path argument is required for the first call")
        else:
            config['html_template_path'] = template_path
        if not output_path:
            if 'html_output_path' in config:
                output_path = config['html_output_path']
            else:
                raise RuntimeError("output_path argument is required for the first call")
        else:
            config['html_output_path'] = output_path
    template_file = open(template_path, "r", encoding="UTF-8")
    html = template_file.read()
    team_names_raw = results.names()
    ranking_text, team_names = [], []

    for rank, (index, wins) in enumerate(results.rankings):
        classes = ["rank", "rank-" + str(rank)]
        ranking_text.append("<li id=\"team-" + str(rank) + "\" class=\"" +
            " ".join(classes) + "\">" + str(rank+1) +
            ". <strong>" + team_names_raw[index] + "</strong> with " + str(wins) + " wins</li>")
        team_names.append(team_names_raw[index])

    # TODO: This is very hacky, use a template library e.g. Jinja?
    html = html.replace("{%RANKINGS%}", "\n".join(ranking_text))
    import datetime
    html = html.replace("{%TIMESTAMP%}", str(datetime.datetime.now()))
    html = html.replace("{%TEAMS%}", str(team_names))
    html = html.replace("{%WINRATE_MATRIX%}", str(results.array().tolist()))

    output_file = open(output_path, "w", encoding="UTF-8")
    output_file.write(html)
    output_file.close()
