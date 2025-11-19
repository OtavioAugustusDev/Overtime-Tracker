# Overtime Tracker
Sistema gerenciador de horas extras desenvolvido em C com interface gráfica GTK4 e banco de dados MySQL.

<img width="1365" height="716" alt="image" src="https://github.com/user-attachments/assets/59a5b870-e909-43f0-ad97-7413c6f76484" />

## Configuração do Ambiente de Desenvolvimento

### Pré-requisitos

1. **Compilador C**
   - [Guia de instalação](https://syntaxpathways.com/set-up-c-development-environment/)

2. **IDE Code::Blocks 25.03**
   - [Download aqui](https://sourceforge.net/projects/codeblocks/files/Binaries/25.03/Windows/codeblocks-25.03-setup.exe/download)

3. **MySQL Server**
   - [Download aqui](https://dev.mysql.com/downloads/windows/installer/8.0.html)

> **Observação**: Instale todos os programas nos diretórios padrão. Caso contrário, será necessário configurar os caminhos manualmente na IDE.

## Instalação das Dependências

### 1. GTK4 e Utilitários

Abra o terminal **MSYS2 MSYS** e execute:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-toolchain
pacman -S base-devel
pacman -S mingw-w64-x86_64-gtk4
pacman -S mingw-w64-x86_64-glade
pacman -Syu
```

Ao final, verifique se a instalação correu bem executando o seguinte comando no terminal **MSYS2 MINGW64**:

```bash
pkg-config --cflags gtk4
```

> **Observação**: Caso o comando não seja encontrado, repita os passos de instalação novamente.

## Configuração do Banco de Dados

1. Abra o **MySQL Command Line Client**
2. Faça login com a senha do usuário `root`
3. Execute o arquivo de inicialização:

```sql
source C:\Users\SeuUsuario\caminho\para\o\projeto\init.sql
```

4. Configure as credenciais no arquivo `database.h`:

```c
#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "sua_senha_aqui"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306
```

## Estrutura do Projeto

```
overtime-tracker/
├── main.c              # Lógica da aplicação e interface
├── database.c          # Operações de banco de dados
├── database.h          # Declarações de funções do banco
├── interface.c         # Componentes de interface reutilizáveis
├── interface.h         # Declarações de funções da interface
└── init.sql           # Script de inicialização do banco
```

## Compilação e Execução

### Via Code::Blocks

1. Abra o arquivo `Overtime Tracker.cbp` no Code::Blocks
2. Compile o projeto: `Ctrl + F9`
3. Aguarde a mensagem de sucesso na saída
4. Execute o programa: `Ctrl + F10`

### Via Linha de Comando

```bash
gcc -o overtime_tracker main.c database.c interface.c \
    `pkg-config --cflags --libs gtk4` \
    `mysql_config --cflags --libs`

./overtime_tracker
```

## Uso do Sistema

### Credenciais de Teste

O sistema vem com dois usuários pré-cadastrados:

| Usuário | Senha | Tipo | Saldo de Horas |
|---------|-------|------|----------------|
| otavio  | 1234  | GESTOR | 0.00 |
| breno   | 1234  | USER | 25.50 |

### Painel do Funcionário (USER)

Ao fazer login como funcionário, você terá acesso a:

#### Dashboard
- Visualização do saldo atual no banco de horas
- Informações sobre a carga horária semanal
- Lista de todos os requerimentos realizados com seus respectivos status

#### Novo Requerimento
1. Clique no botão "Novo requerimento"
2. Selecione a data desejada para a folga (deve ser uma data futura)
3. Ajuste a quantidade de horas usando o controle deslizante
4. Adicione observações para o gestor (opcional)
5. Clique em "Enviar Requerimento"

**Observações**:
- O botão de requerimento fica desabilitado quando não há saldo de horas extras
- O sistema impede requerimentos para datas passadas
- A quantidade máxima de horas é limitada ao saldo disponível

#### Acompanhamento
- A lista de requerimentos é atualizada automaticamente a cada 2 segundos
- Status possíveis: PENDENTE, APROVADO, NEGADO
- Cada requerimento exibe: data, horas solicitadas, observações e status

### Painel do Gestor (GESTOR)

Ao fazer login como gestor, você terá acesso a:

#### Dashboard do Gestor
- Contador de requerimentos pendentes de análise
- Acesso rápido às funcionalidades administrativas

#### Gerir Funcionários
Permite o gerenciamento completo da equipe:

**Criar Novo Usuário**:
1. Clique em "Criar novo usuário"
2. Preencha os campos obrigatórios:
   - Nome de usuário (único)
   - Senha
   - Cargo (USER ou GESTOR)
   - Horas semanais de trabalho
3. Clique em "Salvar"

**Editar Usuário**:
1. Clique em "Editar" no usuário desejado
2. Modifique as informações necessárias
3. Clique em "Salvar"

**Excluir Usuário**:
1. Clique em "Excluir" no usuário desejado
2. O usuário e todos os seus requerimentos serão removidos

#### Gerir Requerimentos
Permite analisar e responder às solicitações de folga:

**Visualização**:
- Lista completa de todos os requerimentos do sistema
- Informações exibidas: funcionário, data, horas solicitadas, observações e status
- Atualização automática a cada 2 segundos

**Análise de Requerimentos**:
1. Revise os detalhes do requerimento
2. Clique em "Deferir" para aprovar:
   - O status muda para APROVADO
   - O saldo de horas do funcionário é atualizado automaticamente
3. Clique em "Indeferir" para negar:
   - O status muda para NEGADO
   - O saldo de horas do funcionário permanece inalterado

**Observações**:
- Requerimentos já aprovados ou negados não podem ser modificados
- Os botões ficam desabilitados após a decisão
- O saldo é deduzido apenas quando o requerimento é aprovado

## Recursos de Segurança

O sistema implementa diversas medidas de segurança:

- Proteção contra SQL Injection usando `mysql_real_escape_string()`
- Validação de entrada do usuário em todos os formulários
- Tratamento adequado de erros em operações de banco de dados
- Verificação de ponteiros NULL antes de uso
- Gerenciamento correto de memória com liberação de recursos
- Escape de caracteres especiais em queries SQL

> **Importante**: Para uso em produção, é altamente recomendado implementar hash de senhas usando algoritmos como BCrypt ou Argon2.

## Solução de Problemas

### Erro de Compilação

**Problema**: `fatal error: gtk/gtk.h: No such file or directory`

**Solução**: Verifique se o GTK4 foi instalado corretamente executando:
```bash
pkg-config --modversion gtk4
```

### Erro de Conexão com MySQL

**Problema**: `Erro ao conectar: Access denied for user`

**Solução**: Verifique as credenciais no arquivo `database.h` e certifique-se de que o MySQL Server está em execução.

### Erro de Locale (Vírgula/Ponto Decimal)

**Problema**: `Column count doesn't match value count at row 1`

**Solução**: O sistema já está configurado para usar ponto decimal em valores numéricos através de `setlocale(LC_NUMERIC, "C")` no `main()`.

### Sistema Fechado para Requerimentos

O sistema possui um horário limite configurado. Para modificar, altere a constante no `main.c`:
```c
#define SYSTEM_LOCK_TIME 999  // Hora limite (formato 24h)
```

## Referências

- [Instalação do GNU COMPILER COLLECTION](https://syntaxpathways.com/set-up-c-development-environment/)
- [Instalação e configuração da biblioteca GTK4 na IDE Code::Blocks](https://www.treinaweb.com.br/blog/criando-interfaces-graficas-no-c-com-gtk/)
- [Instalação e configuração do connector MySQL na IDE Code::Blocks](https://www.treinaweb.com.br/blog/utilizando-o-mysql-em-uma-aplicacao-c/)
- [Instalação das dependências de compilação do GTK4](https://www.treinaweb.com.br/blog/criando-uma-aplicacao-c-com-gtk-no-codeblocks/)
- [Documentação da biblioteca GTK4](https://docs.gtk.org/gtk4/)
- [Utilização da linha do comando do Cliente MySQL](https://www.treinaweb.com.br/blog/primeiros-passos-com-mysql)
- [Conversão de tipos em C](https://www.geeksforgeeks.org/c/c-typecasting/)
- [Definição de estruturas em C](https://www.geeksforgeeks.org/c/structures-c/)
- [Proteção contra SQL Injection](https://dev.mysql.com/doc/c-api/8.0/en/mysql-real-escape-string.html)
- [Gerenciamento de memória em GLib](https://docs.gtk.org/glib/memory.html)

## Contribuindo

Para contribuir com o projeto:

1. Faça um fork do repositório
2. Crie uma branch para sua feature (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças (`git commit -m 'Adiciona MinhaFeature'`)
4. Push para a branch (`git push origin feature/MinhaFeature`)
5. Abra um Pull Request

## Licença

Este projeto foi desenvolvido como Projeto Integrador da Universidade Vila Velha.
