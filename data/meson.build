resource_files = files([
    'ui/app_window.ui',
    'ui/scan_page.ui',
])

resources = gnome.compile_resources ('resources', 'receipts.gresource.xml',
                                     dependencies: resource_files,
                                     source_dir: '.',
                                     c_name: 'resources')
