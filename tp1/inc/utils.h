/**
 * @file utils.h
 * @brief Trabajo practico 1. Sistemas Operativos de Proposito General.
 * @author Gonzalo G. Fernandez
 *
 */

#ifndef INC_UTILS_H
#define INC_UTILS_H

#define BUFFER_SIZE 300
#define MSG_PREFIX_LEN 5
#define MSG_SIGUSR_LEN 8

const char *pipe_dir_path = "tmp"; /*!> Directory path for named PIPE */
const char *log_dir_path = "log";  /*!> Directory path for named PIPE */

const char *pipe_name = "tmp/named_fifo";

const char *data_log_path = "log/log.txt";
const char *sign_log_path = "log/signals.txt";

const char *data_msg_prefix = "DATA:"; /*!> Data message prefix */
const char *sign_msg_prefix = "SIGN:"; /*!> Signal message prefix */
const char *sigusr1_msg = "SIGN:1\n";  /*!> SIGUSR1 messages to log */
const char *sigusr2_msg = "SIGN:2\n";  /*!> SIGUSR2 messages to log */

#endif /* INC_UTILS_H */
