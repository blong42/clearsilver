(defvar cs-font-lock-keywords
  (list
    '("<\\?cs[^\\?]*\\?>"  0 font-lock-keyword-face t)
   '("<\\?cs +\\(call\\|each\\|if\\|alt\\|var\\|include\\)" 1  font-lock-keyword-face t) 
   '("<\\?cs +\\(/each\\|/if\\|/alt\\|/var\\|/include\\)" 1  font-lock-keyword-face t) 

   ; variable names
   '("<\\?cs +var:\\([_0-9a-zA-Z\.]+\\)[^\\?]+\\?>" 1  font-lock-variable-name-face t) 
   '("<\\?cs +alt:\\([_0-9a-zA-Z\.]+\\)[^\\?]+\\?>" 1  font-lock-variable-name-face t) 
   '("<\\?cs +each:\\([_0-9a-zA-Z\.]+\\)[^\\?]+\\?>" 1  font-lock-variable-name-face t) 

   ; string
   '("<\\?cs[^\"\\?]+\\(\"[^\"]+\"\\)[^\\?]+\\?>"  1 font-lock-string-face t)

))
 
(defvar cs-mode-map ())

(if (not cs-mode-map)
    (progn
      (setq cs-mode-map (make-sparse-keymap))
      (define-key cs-mode-map "\C-c\C-i" 'cs-insert-tag)
      ))

(defun cs-mode nil
  "ClearSilver mode"

  (interactive)
  (setq major-mode 'cs-mode)
  (setq mode-name  "CS")
  (use-local-map cs-mode-map)

  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults
       '((cs-font-lock-keywords) t))

  (setq font-lock-keywords cs-font-lock-keywords)

  (font-lock-mode 1)

  (run-hooks 'cs-mode-hook)

)

(defun cs-insert-tag ()
  (interactive)
  (insert "<?cs  ?>")
  (backward-char 3)
)

