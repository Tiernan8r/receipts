project('receipts', ['cpp', 'c'],
          version: '0.1.0',
    meson_version: '>= 0.62.0',
  default_options: [ 'warning_level=2', 'werror=false', 'cpp_std=gnu++2a', ],
)

# Paths
datadir = join_paths (get_option ('prefix'), get_option ('datadir'))

# Dependencies
gnome = import('gnome')
gtkmm_dep = dependency('gtkmm-4.0')
sane_dep = dependency('sane-backends')

# Project dirs
subdir ('data')
subdir('src')
