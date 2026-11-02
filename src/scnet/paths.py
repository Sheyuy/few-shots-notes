"""Resolve the few-shots clone and problem directories.

Set FEW_SHOTS_REPO when the clone is not the default location.
"""
import os
from pathlib import Path

REPO = Path(os.environ.get('FEW_SHOTS_REPO', '/Users/shiny/.sclaw/workspace/few-shots'))
P004 = REPO / 'problems' / 'p004-shannon-capacity-c7'
P005 = REPO / 'problems' / 'p005-cap-set-8d'
