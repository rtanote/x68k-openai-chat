; chat.el - microEmacs AI Chat Integration
;
; Usage:
;   1. Copy this file to your microEmacs directory
;   2. Add to your emacs.rc
;      autoload ask-ai &cat %el-load-path "chat.el" (in custom/autoload_mac.el)
;      macro-to-key  ask-ai  M-^a (in custom/keybind.el)
;   3. Use ESC a to ask AI a question
;
; Commands(example):
;   ESC ^a     - Ask AI (input in minibuffer, insert response)

; Ask AI - input question in minibuffer
store-procedure ask-ai
    set %query @"Ask AI: "
    write-message %query
    !if &seq %query ""
        !return
    !endif
    set %oldbuf $cbufname
    ; Show question
    insert-string "> "
    insert-string %query
    insert-string "~n"
    ; Execute CHAT.X and insert output using pipe-command
    set %cmd &cat "chat " %query
    pipe-command %cmd
    beginning-of-file
    set-mark
    end-of-file
    copy-region
    select-buffer %oldbuf
    yank
    insert-string "~n"
    delete-buffer "*pipe*"
    delete-other-windows
!endm