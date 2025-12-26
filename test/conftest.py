import pytest

def pytest_addoption(parser):
    parser.addoption(
        "--jlq",
        action="store",
        default=None,
        help="Path to the jlq binary to test"
    )

@pytest.fixture
def jlq_bin(request):
    return request.config.getoption("--jlq")
