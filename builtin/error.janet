# vim: sw=2 ts=2 sts=2 expandtab

(def errorstr " error ")

; Function to initialize the error screen
(defn I_ERROR_init []
  ; Function to log messages for debugging
  (defn log [message]
    (print (string/format "[LOG] %s\n" message)))

  ; Function to handle errors
  (defn handle_error [error_message]
    (log error_message)
    (throw (error error_message)))

  ; Log the start of the error screen initialization
  (log "Starting error screen initialization...")

  ; Reset font by copying font data from bank 1 to bank 0
  (swibnk 1)
  (log "Switched to bank 1")
  (def font-start 0x4040)
  (def font-end 0x52a0)
  (def font-size (- font-end font-start))
  (def fonts (peek font-start font-size))
  (if (nil? fonts)
    (handle_error "Failed to read fonts from bank 1"))
  (swibnk 0)
  (log "Switched back to bank 0")
  (poke font-start fonts)
  (log "Font data written to bank 0")

  ; Fill the screen with random characters, excluding lowercase
  (log "Filling screen with random characters...")
  (loop [y :range [0 height]]
    (loop [x :range [0 width]]
      (color (+ 1 (rand 14))) ; Set random color
      (c7put x y (string/from-bytes (+ 32 (rand 56))))) ; Place random character from ASCII 32 to 87
  )

  ; Draw the error text centered on the screen
  (color 1)
  (let [x (- (// width 2) (// (length errorstr) 2))
        y (// height 2)
        spcs (string/repeat " " (length errorstr))]
    (log "Drawing error text...")
    (c7put x (- y 1) spcs)
    (c7put x y errorstr)
    (c7put x (+ y 1) spcs))

  ; Log the completion of error screen initialization
  (log "Error screen initialization completed successfully")
)

; Function to update the error screen each step
(defn I_ERROR_step []
  (log "Updating error screen step...")
  (delay 1) ; Delay for 1 second to control update speed
  (def sparsity (* (+ (// (ticks) 7) 1) 7)) ; Calculate sparsity based on ticks

  ; Randomly modify memory locations to simulate errors
  (loop [i :range [(+ 0x4040 (* 1 49)) (+ 0x4040 (* 56 49))]]
    (poke i (if (= (rand sparsity) 0) 1 0))) ; Randomly poke memory with 1s and 0s
  (log "Error screen step update completed")
)

; Example usage of the I_ERROR_init and I_ERROR_step functions
(I_ERROR_init)
(loop []
  (I_ERROR_step))
