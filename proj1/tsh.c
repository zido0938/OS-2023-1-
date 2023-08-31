/*
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */

/*
 * Last modified by Ji Hyeon Do
 * Data : 2023/03/25
 * Name : Ji Hyeon Do
 * Department(Division) : Computer science & engineering
 * Student_Number: 2021004866
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80                          /* 명령어의 최대 길이 */

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다. 
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */

static void cmdexec(char *cmd, int fd_in, int fd_out) {     
    char *argv[MAX_LINE/2+1];           /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;                       /* 인자의 개수 */
    char *p, *q;                        /* 명령어를 파싱하기 위한 변수 */
 
    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    do {
	/*
            * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
         */ 
        q = strpbrk(p, " \t\'\"");
	/*
            * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
         */
        if (q == NULL || *q == ' ' || *q == '\t') {
            q = strsep(&p, " \t");
            if (*q) argv[argc++] = q;
	/*
            * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
            * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
            * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        } else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
        } 
	 /*
             * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
             * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
             * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
          */
	else {
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
        }
    } while (p);
    argv[argc] = NULL;

	/*
	 *  표준 입출력 리다이렉션 '<' 를 위한 코드
	 */
     for (int i = 0; argv[i] != NULL; i++) {
        if (!strcmp(argv[i], "<")) {
            fd_in = open(argv[i+1], O_RDONLY);
            if (fd_in == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            argv[i] = NULL;
            i++;
        } 
	/*
	 *  표준 입출력 리다이렉션 '>' 를 위한 코드
	 */
	else if (!strcmp(argv[i], ">")) {
            fd_out = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            argv[i] = NULL;
            i++;
	    }
     }

    // 리다이렉션 예외처리
    if (fd_in != STDIN_FILENO) {
        if (dup2(fd_in, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(fd_in);
    }

    if (fd_out != STDOUT_FILENO) {
        if (dup2(fd_out, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(fd_out);
    }

    /*
     * argv에 저장된 명령어를 실행한다.
     */
    if (argc > 0) execvp(argv[0], argv);
}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */

int main(void)
{
    char cmd[MAX_LINE+1];      /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    int background;             /* 백그라운드 실행 유무 */
    int fd_in, fd_out;            /* 파이프 기능을 위한 변수(디스크립터) */
    char *p;                   /* 명령어를 파싱하기 위한 변수 */


	/*
            * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
    	 */
    while (true) {
	/*
            * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
	/*
            * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
	/*
            * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
            * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
            * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
	/*
            * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
	 /*
             * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
          */
        p = strchr(cmd, '&');
        if (p != NULL) {
            background = 1;
            *p = '\0';
        }
        else
            background = 0;
        /* 다중 파이프를 위한 명령어 파싱. */
        char *cmds[MAX_LINE/2+1];       /* 파이프를 위한 버퍼 생성 */
        int num_cmds = 0;              /* 파이프의 개수를 세기 위한 변수 */
        cmds[num_cmds++] = cmd; 
     
        p = strchr(cmd, '|');
        while (p != NULL) {
            *p = '\0';
            cmds[num_cmds++] = p + 1;
            p = strchr(p + 1, '|');
        }

        int pipefds[num_cmds - 1][2];     /* 파이프 생성 */
        for (int i = 0; i < num_cmds - 1; i++) {
            if (pipe(pipefds[i]) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

       pid_t child_pids[num_cmds];       /* 자식pid 생성(파이프) */
        for (int i = 0; i < num_cmds; i++) {
            fd_in = i == 0 ? STDIN_FILENO : pipefds[i-1][0];
            fd_out = i == num_cmds - 1 ? STDOUT_FILENO : pipefds[i][1];

            if ((child_pids[i] = fork()) == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } 
		/* 
		 * 자식 프로세스
		*/
		else if (child_pids[i] == 0) {
                if (fd_in != STDIN_FILENO) {
                    if (dup2(fd_in, STDIN_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(fd_in);
                }

                if (fd_out != STDOUT_FILENO) {
                    if (dup2(fd_out, STDOUT_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(fd_out);
                }

                for (int j = 0; j < num_cmds - 1; j++) {
                    close(pipefds[j][0]);
                    close(pipefds[j][1]);
                }

                cmdexec(cmds[i], STDIN_FILENO, STDOUT_FILENO);   
                exit(EXIT_SUCCESS);
            }
        }

        for (int i = 0; i < num_cmds - 1; i++) {
            close(pipefds[i][0]);
            close(pipefds[i][1]);
        }
	/*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        if (!background) {
            for (int i = 0; i < num_cmds; i++) {
                waitpid(child_pids[i], NULL, 0);
            }
        }
    }

    return 0;
}

	

