# mIRC - Redes

Projeto da Disciplina **SSC0142 - Redes de Computadores (2022)**, no qual implementaremos uma versão simplificada do protocolo IRC (Internet Relay Chat).

> Atualmente, o projeto encontra-se na versão para a primeira entrega `Módulo 1 - Implementação de Sockets (entrega 07/06/2022)`

## Autores

- Lourenço de Salles Roselino &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| 11796805
- Marco Antônio Ribeiro de Toledo | 11796419
- Melissa Motoki Nogueira &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;| 5588687

> O código foi compilado utilizando `gcc versão 7.5.0`, com `target = Linux GNU x86_64`. O código foi desenvolvido em `C++17`, usando a `flag -std=gnu++17` durante a compilação.

## Vídeo

O vídeo de apresentação do projeto pode ser acessado através do YouTube ou Google Drive.

## Como usar

Para testar o código, compile-o com o comando `make all`.

O servidor será iniciado na porta `6667` (essa configuração está contida na variável `static string PORT{}` no arquivo ![`Conn.hpp`](src/Utils/Conn.hpp)). O programa cliente também será iniciado.

## Comandos disponíveis
> Na versão para a segunda entrega `Módulo 2 - Comunicação entre múltiplos clientes e servidor (entrega 19/07/2022)`

`/connect` - Estabelece a conexão com o servidor;<br>
`/quit` - O cliente fecha a conexão e fecha a aplicação;<br>
`/ping` - O servidor retorna "pong" assim que receber a mensagem.<br>

> Na versão para a entrega final `Módulo 3 - Implementação de múltiplos canais (entrega 19/07/2022)`

`/join nomeCanal` - Entra no canal;<br>
`/nickname apelidoDesejado` - O cliente passa a ser reconhecido pelo apelido especificado;<br>
`/ping` - O servidor retorna "pong" assim que receber a mensagem.<br>
<br>**Comandos apenas para administradores de canais:**<br>
`/kick nomeUsurio` - Fecha a conexão de um usuário especificado;<br>
`/mute nomeUsurio` - Faz com que um usuário não possa enviar mensagens neste canal;<br>
`/unmute nomeUsurio` - Retira o mute de um usuário;<br>
`/whois nomeUsurio` - Retorna o endereço IP do usuário apenas para o administrador.

> Para mandar mensagens no servidor, apenas digite normalmente no terminal e aperte **ENTER**.
