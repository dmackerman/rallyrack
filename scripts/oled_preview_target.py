import os
import subprocess


def run_program(source, target, env):
    program_path = env.subst("$PROGPATH")
    run_args = env.GetProjectOption("run_args", "")
    args = run_args.split() if run_args else []
    command = [program_path] + args
    subprocess.check_call(command)


env.AddCustomTarget(
    name="run",
    dependencies="$PROGPATH",
    actions=run_program,
    title="Run OLED preview",
    description="Run the native OLED preview program",
)
