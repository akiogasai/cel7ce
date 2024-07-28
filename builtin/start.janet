# vim: sw=2 ts=2 sts=2 expandtab
#
# Display a nice animation.

; Initialize the animation setup
(defn I_START_init []
  ; Copy over palette data from bank 1 to the current bank (bank 0)
  (swibnk 1) ; Switch to memory bank 1
  (def palette (peek 0x4000 (- 0x4040 0x4000))) ; Read the palette data from memory addresses 0x4000 to 0x4040
  (swibnk 0) ; Switch back to memory bank 0
  (poke 0x4000 palette) ; Write the palette data to memory address 0x4000 in the current bank

  ; Put random characters on the screen with random colors
  (loop [y :range [0 height]] ; Loop through each row of the screen
    (loop [x :range [0 width]] ; Loop through each column of the screen
      (color (+ 1 (rand 14))) ; Set a random color (between 1 and 14, as 0 is typically black)
      (c7put x y (string/from-bytes (+ 20 (rand 96))))) ; Place a random character (from ASCII 20 to 115) at the x, y position
  )
)

; Update the animation each step
(defn I_START_step []
  (delay 0.1) ; Delay for 0.1 seconds to control the speed of the animation
  (def n (ticks)) ; Get the number of ticks since the program started
  (cond
    ; For the first 5 ticks, randomly set memory locations to 1 or 0
    (< n 5)
      (loop [i :range [0x4040 0x52a0]] ; Loop through memory range 0x4040 to 0x52a0
        (poke i (if (= (rand (* n n)) 0) 1 0))) ; Randomly set memory locations to 1 or 0 based on the tick count

    ; On the 6th tick, clear the screen with spaces
    (= n 6)
      (do
        (color 0) ; Set color to black
        (fill 0 0 width height " ")) ; Fill the screen with black color

    ; After the 8th tick, switch to a different mode or state
    (> n 8)
      (do
        (swimd 1) ; Switch to another display mode
        (delay 0.3)) ; Delay to control the timing of the mode switch
  )
)
