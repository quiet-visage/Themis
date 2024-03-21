#pragma once

enum editor_cmd {
    editor_cmd_none = -1,
    editor_cmd_move_char_left,
    editor_cmd_move_char_right,
    editor_cmd_move_line_up,
    editor_cmd_move_line_down,
    editor_cmd_move_end_of_line,
    editor_cmd_move_beg_of_line,
    editor_cmd_copy,
    editor_cmd_paste,
    editor_cmd_cut,
    editor_cmd_begin_mode_selection,
    editor_cmd_end_mode_selection,
    editor_cmd_begin_mode_search,
    editor_cmd_end_mode_search,
    editor_cmd_delete,
    editor_cmd_backspace,
    editor_cmd_next_search_match,
    editor_cmd_previous_search_match,
    editor_cmd_undo,
    editor_cmd_redo,
    editor_cmd_delete_rest_of_line,
    editor_cmd_move_word_right,
    editor_cmd_move_word_left,
    editor_cmd_move_buffer_end,
    editor_cmd_move_buffer_begin,
    editor_cmd_count
};

enum line_editor_cmd {
    line_editor_cmd_none = -1,
    line_editor_cmd_move_char_left,
    line_editor_cmd_move_char_right,
    line_editor_cmd_move_end_of_line,
    line_editor_cmd_move_beg_of_line,
    line_editor_cmd_copy,
    line_editor_cmd_paste,
    line_editor_cmd_cut,
    line_editor_cmd_begin_mode_selection,
    line_editor_cmd_end_mode_selection,
    line_editor_cmd_delete,
    line_editor_cmd_backspace,
    line_editor_cmd_undo,
    line_editor_cmd_redo,
    line_editor_cmd_delete_rest_of_line,
    line_editor_cmd_move_word_right,
    line_editor_cmd_move_word_left,
    line_editor_cmd_count
};

enum pane_cmd {
    pane_cmd_none = -1,
    pane_cmd_split_vertical,
    pane_cmd_split_horizontal,
    pane_cmd_close_pane,
    pane_cmd_focus_next,
    pane_cmd_focus_prev,
    pane_cmd_focus_up,
    pane_cmd_focus_down,
    pane_cmd_focus_left,
    pane_cmd_focus_right,
    pane_cmd_count
};

enum main_cmd {
    main_cmd_none = -1,
    main_cmd_open_file_picker,
    main_cmd_open_buffer_picker,
    main_cmd_close,
    main_cmd_count
};

enum menu_cmd {
    menu_cmd_none = -1,
    menu_cmd_move_up,
    menu_cmd_move_down,
    menu_cmd_count
};
enum file_picker_cmd {
    file_picker_cmd_none = -1,
    file_picker_cmd_previous_dir,
};

enum file_editor_cmd {
    file_editor_cmd_none = -1,
    file_editor_cmd_save,
};
