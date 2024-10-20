#pragma once

/**
 * @brief Writes the C string pointed by `pattern` to the standard output
 *        If `pattern` includes format specifiers (subsequences beginning with %),
 *        the additional arguments following the `pattern` are formatted and inserted in
 *        the resulting string replacing their respective specifiers.
 *
 *        Specifier | Output                     | Example
 *        --------- | -------------------------- | -------
 *        %d %i     | Signed decimal integer     | 420
 *        %o        | Signed octal decimal       | 644
 *        %x        | Signed hexadecimal integer | 1A4
 *        %b        | Signed binary integer      | 110100100
 *        %c        | Single character           | a
 *        %s        | Null-terminated string     | Hello!
 *        %%        | A literal '%' character    | %
 *
 *        If  a specific specifiers sequence is not recoginez it will be
 *        ignored and skipped, without producing any output.
 *
 * @param[in] pattern C string that contains the text to be written, with optional embedded format specifiers.
 * @param[in] ...     Depending on the format string, the function may expect a sequence of additional arguments.
 *
 * @return Returns a pointer to the buffer that was written to.
 */
void kprintf(const char* pattern, ...);
