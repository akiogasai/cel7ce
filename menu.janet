# vim: sw=2 ts=2 sts=2 expandtab

(import janet)
(import os)
(import math)

# Define menu items
(var menu [
  ["About"]
  ["Reload"]
  ["Resume"]
  ["Quit"]
])
(var sel 0)

# Colors and styling
(def menu-color 0xFFFFFF)
(def highlight-color 0x00FF00)
(def background-color 0x000000)
(def font-size 16)

# Initialize GUI components
(defn init []
  (clear-screen)
  (set-font "ModernFont" font-size)
  (draw-menu))

# Clear the screen
(defn clear-screen []
  (color background-color)
  (fill-rect 0 0 (screen-width) (screen-height)))

# Set the font
(defn set-font [name size]
  ;; Assuming a hypothetical set-font function for the fantasy console
  (os/execute (string "setfont " name " " size)))

# Draw the menu
(defn draw-menu []
  (clear-screen)
  (color menu-color)
  (put-text 10 10 "Menu:" menu-color)
  (loop [i :range [0 (length menu)]]
    (if (= sel i)
      (highlight-menu-item i)
      (normal-menu-item i))
    (put-text 20 (+ 40 (* i 40)) ((menu i) 0))))

# Highlight selected menu item
(defn highlight-menu-item [i]
  (color highlight-color)
  (draw-rectangle 15 (+ 30 (* i 40)) 150 30 highlight-color)
  (color menu-color))

# Normal menu item
(defn normal-menu-item [i]
  (color menu-color)
  (draw-rectangle 15 (+ 30 (* i 40)) 150 30 background-color))

# Handle keydown events
(defn keydown [k]
  (cond
    (or (= k "k") (= k "up"))
    (if (> sel 0) (-- sel))
    (or (= k "j") (= k "down"))
    (if (< sel (- (length menu) 1)) (++ sel)))
  (draw-menu))

# Handle mouse events
(defn mouse [type _ _ y]
  (if (= type "motion")
    (do
      (def ry (math/floor y))
      (var nsel (math/floor (/ (- ry 30) 40)))
      (if (and (>= nsel 0) (< nsel (length menu)))
        (set sel nsel))
      (draw-menu)))
  (if (= type "click")
    (do
      (def ry (math/floor y))
      (var nsel (math/floor (/ (- ry 30) 40)))
      (if (and (>= nsel 0) (< nsel (length menu)))
        (execute-menu-item nsel)))))

# Execute selected menu item
(defn execute-menu-item [index]
  (match index
    0 (show-about)
    1 (reload-game)
    2 (resume-game)
    3 (quit-game)
    (_ (resume-game))))

# Show about information
(defn show-about []
  (clear-screen)
  (put-text 10 10 "Cel7-CE Version 1.0" menu-color)
  (put-text 10 50 "Created by: Your Name" menu-color)
  (os/sleep 2)  # Pause for 2 seconds
  (draw-menu))

# Reload game
(defn reload-game []
  (clear-screen)
  (put-text 10 10 "Reloading game..." menu-color)
  (os/sleep 2)  # Simulate reload time
  (draw-menu))

# Resume game
(defn resume-game []
  (clear-screen)
  (put-text 10 10 "Resuming game..." menu-color)
  (os/sleep 2)  # Simulate resume time
  (draw-menu))

# Quit game
(defn quit-game []
  (clear-screen)
  (put-text 10 10 "Quitting game..." menu-color)
  (os/sleep 2)  # Simulate quit time
  (os/exit 0))

# Display text with color
(defn put-text [x y text color]
  ;; Assuming a hypothetical function for text display
  (os/execute (string "puttext " x " " y " " text " " color)))

# Draw rectangle with color
(defn draw-rectangle [x y width height color]
  ;; Assuming a hypothetical function for drawing rectangles
  (os/execute (string "drawrect " x " " y " " width " " height " " color)))

# Initialize the menu
(init)
