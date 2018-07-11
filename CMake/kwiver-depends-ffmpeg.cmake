# Optional find and confgure FFmpeg dependency

option( KWIVER_ENABLE_FFMPEG
  "Enable FFMPEG dependent code and plugins (Arrows)"
  ${fletch_ENABLED_FFmpeg}
  )

if( KWIVER_ENABLE_FFMPEG )
  find_package( FFMPEG REQUIRED )
endif( KWIVER_ENABLE_FFMPEG )
