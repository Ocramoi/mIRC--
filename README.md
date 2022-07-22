# mIRC - Internet Relay Chat

Projeto da Disciplina **SSC0142 - Redes de Computadores (2022)**, no qual implementaremos uma versão simplificada do protocolo IRC (Internet Relay Chat).


## Autores

- Lourenço de Salles Roselino &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| 11796805
- Marco Antônio Ribeiro de Toledo &nbsp;&nbsp;&nbsp;| 11796419
- Melissa Motoki Nogueira &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| 5588687

> O código foi compilado utilizando `clang++ versão 11.0.0`, com `target = Linux GNU x86_64`. O código foi desenvolvido em `C++17`, usando a `flag -std=gnu++17` durante a compilação.

## Vídeo

O vídeo de apresentação do projeto pode ser acessado através do YouTube ou Google Drive.

## Árvore de arquivos do projeto
    .
    ├── README.md
    ├── build
    └── src
        ├── Makefile
        ├── main.cpp
        └── Utils
                └── Conn.hpp
                └── Utils.cpp
                └── Utils.hpp
        └── Client
                └── Client.cpp
                └── Client.hpp
        └── Channel
                └── Channel.cpp
                └── Channel.hpp
        └── Server
                └── Server.cpp
                └── Server.hpp
        
## Como usar

Para testar o código, compile-o com o comando `make` (ou em modo de debug com `make debug`).

O servidor será iniciado na porta `6667` (essa configuração está contida na variável `static string PORT{}` no arquivo ![`Conn.hpp`](src/Utils/Conn.hpp)). O programa cliente também será iniciado.

## Comandos disponíveis

<br>**Comandos comuns a todos os usuários:**<br>
`/connect` - Estabelece a conexão com o servidor;<br>
`/quit` - O cliente fecha a conexão e fecha a aplicação;<br>
`/ping` - O servidor retorna "pong" assim que receber a mensagem.<br>
`/join nomeCanal` - Entra no canal;<br>
`/nickname apelidoDesejado` - O cliente passa a ser reconhecido pelo apelido especificado;<br>
`/ping` - O servidor retorna "pong" assim que receber a mensagem.<br>

<br>**Comandos apenas para administradores de canais:**<br>
`/kick nomeUsurio` - Fecha a conexão de um usuário especificado;<br>
`/mute nomeUsurio` - Faz com que um usuário não possa enviar mensagens neste canal;<br>
`/unmute nomeUsurio` - Retira o mute de um usuário;<br>
`/whois nomeUsurio` - Retorna o endereço IP do usuário apenas para o administrador.

> Para mandar mensagens no servidor, apenas digite normalmente no terminal e aperte **ENTER**.

## [Vídeos demonstrativos](https://drive.google.com/drive/folders/1BgSQKQgKqrTYgSKYwyWIAmbiJKl1vq3K?usp=sharing)
- [Introdução ao projeto](https://drive.google.com/file/d/1RyqCuXUYl2b_GpqYlP0PVoPGghQf896W/view?usp=sharing)
- [Demonstração do código](https://drive.google.com/file/d/1y3svThCtpExnWe2rklY5v8YA8MJeJs-C/view?usp=sharing)
- [Execução da aplicação](https://drive.google.com/file/d/1a0xV5xIwSjexgVPJdLCmaV-gh8MLWFwF/view?usp=sharing)
