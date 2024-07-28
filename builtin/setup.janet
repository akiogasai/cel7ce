# vim: sw=2 ts=2 sts=2 expandtab
#
# Initialize palette, fonts, color, etc.
# (Palette was probably already initialized in start.janet, but
# we're doing it here again anyway...)

(defn I_SETUP_init []
  ; Define memory addresses for palette and data
  (def palette-start 0x4000)
  (def data-end 0x52a0)
  (def data-size (- data-end palette-start))

  ; Function to log messages for debugging
  (defn log [message]
    (print (string/format "[LOG] %s\n" message)))

  ; Function to handle errors
  (defn handle_error [error_message]
    (log error_message)
    (swimd 3) ; Switch to error mode
    (throw (error error_message)))

  ; Log the start of initialization
  (log "Starting initialization...")

  ; Switch to bank 1 to access initialization data
  (swibnk 1)
  (log "Switched to bank 1")

  ; Read the initialization data from memory bank 1
  (def data (peek palette-start data-size))
  (if (nil? data)
    (handle_error "Failed to read data from bank 1"))

  ; Switch back to bank 0 to write the initialization data
  (swibnk 0)
  (log "Switched back to bank 0")

  ; Write the initialization data to memory bank 0
  (poke palette-start data)
  (log "Initialization data written to bank 0")

  ; Set the default drawing color to 1 (usually white)
  (color 1)
  (log "Default color set to 1")

  ; Check for load errors and switch to the appropriate mode
  (if (lderr)
    (do
      (log "Load error detected, switching to error mode")
      (swimd 3)) ; Switch to error mode
    (do
      (log "No load errors, switching to normal mode")
      (swimd 2))) ; Switch to normal mode

  ; Log the end of initialization
  (log "Initialization completed successfully")
)

; Example usage of the I_SETUP_init function
(I_SETUP_init)
