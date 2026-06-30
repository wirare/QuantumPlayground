from qiskit_ibm_runtime import QiskitRuntimeService

QiskitRuntimeService.save_account(
    token="IMB-API-TOKEN",
    instance="INSTANCE-TOKEN",
    set_as_default=True,
    overwrite=True,
)
