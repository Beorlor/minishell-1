#include "../../header/minishell.h"

char **execute_commande_split(ASTNode* node, int *exit_status, int *here_doc, command *cmd)
{
    char **split_nodeValue;

    if (node->inputs)
        *here_doc = redirection_in(node, exit_status, cmd);
    else
        *here_doc = 0;
    if (*here_doc == 2)
        return NULL;
    replaceEnvVars(&node->value, *(cmd->env), exit_status);
    split_nodeValue = ft_split(node->value, ' ');
    split_nodeValue = check_wildcard(split_nodeValue);
    split_nodeValue = clean_quote_all(split_nodeValue);
    if (split_nodeValue[0] == NULL)
    {
        ft_putendl_fd("minishell: : command not found", 2);
        return NULL;
    }
    if (is_builtin(clean_quote(split_nodeValue[0])))
    {
        if (!execute_builtin(cmd, split_nodeValue, node, exit_status))
            return NULL;
    }
    return (split_nodeValue);
}

void execute_command2(ASTNode* node, command *cmd, char **split_nodeValue, pid_t pid)
{
    if (pid == 0)
    {
        handle_child_process(node, cmd);
        if (is_fork_builtin(clean_quote(split_nodeValue[0])))
            execute_fork_builtin(*(cmd->env), *(cmd->export), split_nodeValue);
        execute(split_nodeValue, get_path(*(cmd->env)), *(cmd->env));
    }
    else
    {
        handle_parent_process(node, cmd);
        cmd->pids[cmd->pid_count] = pid;
        (cmd->pid_count)++;
        free_tab(split_nodeValue);
    }
}

void execute_command(ASTNode* node, command *cmd, int *exit_status)
{
    char **split_nodeValue;
    pid_t pid;

    if (node == NULL || node->value == NULL)
        return;
    if (pipe(cmd->p_id) == -1)
        return ((void) perror("pipe"), (void) exit(EXIT_FAILURE));
    if (cmd->fd == -1)
        cmd->fd = cmd->p_id[1];
    split_nodeValue = execute_commande_split(node, exit_status, &(cmd->here_doc), cmd);
    if (split_nodeValue == NULL)
        return ;
    pid = fork();
    if (pid == -1)
        return ((void) perror("fork"), (void) exit(EXIT_FAILURE));
    execute_command2(node, cmd, split_nodeValue, pid);
}

void execute_output_append_command(ASTNode* node, command *cmd, int *exit_status)
{
    if (node->outputs)
    {
        cmd->fd = open_output_append(node);
        if (cmd->fd == -1)
        {
            *exit_status = 1;
            return ;
        }
        execute_command(node, cmd, exit_status);
        close(cmd->fd);
    }
    if (!node->is_last_command)
    {
        cmd->fd = -1;
        execute_command(node, cmd, exit_status);
    }
}