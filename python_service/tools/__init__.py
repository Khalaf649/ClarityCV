"""CLI tools for inspecting models and reporting performance.

Run any of these as a module from the service root::

    cd python_service
    python -m tools.inspect_model trained_model.pkl
    python -m tools.demo --model trained_model.pkl --image face.jpg
    python -m tools.evaluate --model trained_model.pkl --data /path/to/gt_db --no-detect

Running them as plain scripts (``python tools/demo.py ...``) also works
thanks to the ``sys.path`` bootstrap at the top of each file.
"""
