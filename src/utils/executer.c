#include "../header/minishell.h"

#include <string.h>

int ft_strcmp(char *value1, char *value2) {
    while (*value1 && (*value1 == *value2)) {
        value1++;
        value2++;
    }
    return (*(unsigned char *)value1 - *(unsigned char *)value2);
}

// Fonction is_builtin pour vérifier si une commande est intégrée
int is_builtin(char *value) {
    if (!ft_strcmp(value, "cd") ||
        !ft_strcmp(value, "pwd") ||
        !ft_strcmp(value, "unset") ||
        !ft_strcmp(value, "export") ||
        !ft_strcmp(value, "echo") ||
        !ft_strcmp(value, "env") ||
        !ft_strcmp(value, "exit")) {
        return 1;
    }
    return 0;
}

// Fonction get_path pour obtenir le chemin de l'environnement
char *get_path(char **env) {
    while (ft_strncmp("PATH=", *env, 5) != 0 && *env && env)
        env++;
    return (*env + 5);
}

// Fonction check_path pour vérifier si le chemin d'accès est valide
char *check_path(char **paths, char *name) {
    char *join;
    char *tmp;
    int i = 0;
    while (paths[i]) {
        tmp = ft_strjoin(paths[i], "/");
        join = ft_strjoin(tmp, name);
        free(tmp);
        if (access(join, F_OK | X_OK) == 0)
            return (join);
        free(join);
        i++;
    }
    return (NULL);
}

// Fonction pour libérer la mémoire allouée pour un tableau de chaînes de caractères
void free_all(char **tab) {
    char **tmp = tab;
    while (*tab) {
        free(*tab);
        tab++;
    }
    free(tmp);
}

// Fonction pour exécuter une commande avec les paramètres spécifiés
void execute(char **param, char *path, char **env) {
    char **paths;
    char *path1;
    paths = ft_split(path, ':');
    path1 = check_path(paths, clean_quote(param[0]));
    if (!path1) {
        //free_all(paths);
        //free_all(param);
    }
    if (execve(path1, param, env) == -1) {
        //free_all(paths);
        //free_all(param);
    }
    //free_all(paths);
    //free_all(param);
    //free(path1);
}

int execute_builtin(ASTNode *node, char **env, char **param){
  printf("builtin : \n");
  if (ft_strcmp(clean_quote(param[0]), "cd") == 0)
    return (0);
  else if (ft_strcmp(clean_quote(param[0]), "pwd") == 0)
    printf("%s\n", get_cwd());
  else if (ft_strcmp(clean_quote(param[0]), "unset") == 0)
    return 0;
  else if (ft_strcmp(clean_quote(param[0]), "export") == 0)
    return 0;
  else if (ft_strcmp(clean_quote(param[0]), "echo") == 0)
    echo(param);
  else if (ft_strcmp(clean_quote(param[0]), "env") == 0)
    return 0;
  else if (ft_strcmp(clean_quote(param[0]), "exit") == 0)
    return 0;
  else 
    return 1;
  exit(0);
}

// Fonction pour exécuter un nœud de l'arbre syntaxique abstrait (AST)
void exec(ASTNode* node, char **env, int test, int test2, int* pids, int* pid_count) {
    char **split_nodeValue;
    int p_id[2];
    pid_t pid;
    if (node == NULL || node->value == NULL) {
        return ;
    }
    if (pipe(p_id) == -1)
        exit(0);
    split_nodeValue = ft_split(node->value, ' ');
    pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        if (!(node->is_last_command)) {
            dup2(p_id[1], 1);
            close(p_id[0]);
        } else {
            dup2(test, 1);
            close(test);
        }
        if (is_builtin(clean_quote(split_nodeValue[0])))
          execute_builtin(node, env, split_nodeValue);
        execute(split_nodeValue, get_path(env), env);
    } else {
        if (!(node->is_last_command)) {
            close(p_id[1]);
            dup2(p_id[0], 0);
        } else {
            dup2(test2, STDIN_FILENO);
            close(test2);
        }
        pids[*pid_count] = pid;
        (*pid_count)++;
    }
}

// Fonction récursive pour traiter les nœuds de l'arbre syntaxique abstrait (AST)
void processBinaryTree2(ASTNode* node, char **env, int test, int test2, int* pids, int* pid_count) {
    if (node == NULL) return;
    processBinaryTree2(node->left, env, test, test2, pids, pid_count);
    if (node->type == NODE_COMMAND) {
        exec(node, env, test, test2, pids, pid_count);
    }
    processBinaryTree2(node->right, env, test, test2, pids, pid_count);
}

// Fonction pour étendre les arbres de commandes et les exécuter
void expandCommandTrees2(StartNode* startNode, char **env) {
    if (!startNode->hasLogical) {
        int test = dup(STDOUT_FILENO);
        int test2 = dup(STDIN_FILENO);
        int max_pids = 1024;
        int* pids = malloc(max_pids * sizeof(int));
        int pid_count = 0;
        processBinaryTree2(startNode->children[0]->left, env, test, test2, pids, &pid_count);
        for (int i = 0; i < pid_count; i++) {
            waitpid(pids[i], NULL, 0);
        }
        free(pids);
    } else {
        for (int i = 0; i < startNode->childCount; i++) {
            if (startNode->children[i]->left) {
                int test = dup(STDOUT_FILENO);
                int test2 = dup(STDIN_FILENO);
                int max_pids = 1024;
                int* pids = malloc(max_pids * sizeof(int));
                int pid_count = 0;
                processBinaryTree2(startNode->children[i]->left, env, test, test2, pids, &pid_count);
                for (int j = 0; j < pid_count; j++) {
                    waitpid(pids[j], NULL, 0);
                }
                free(pids);
            }
            if (i == 0 && startNode->children[i]->right) {
                int test = dup(STDOUT_FILENO);
                int test2 = dup(STDIN_FILENO);
                int max_pids = 1024;
                int* pids = malloc(max_pids * sizeof(int)); 
                int pid_count = 0;
                processBinaryTree2(startNode->children[i]->right, env, test, test2, pids, &pid_count);
                for (int j = 0; j < pid_count; j++) {
                    waitpid(pids[j], NULL, 0);
                }
                free(pids);
            }
        }
    }
}

// Fonction principale pour exécuter le nœud de départ
void executer(StartNode* startNode, char **env) {
    expandCommandTrees2(startNode, env);
}