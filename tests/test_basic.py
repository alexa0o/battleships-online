# Start via `make test-debug` or `make test-release`
async def test_basic(service_client):
    response = await service_client.get('/regnewgame')
    assert response.status == 200
    assert response.text == '0'

    response = await service_client.get('/regnewgame')
    assert response.status == 200
    assert response.text == '1'

    sleep(5)

    response = await service_client.get('/regstatus?reg_id=0')
    assert response.status == 200
    assert response.text == '1'

    response = await service_client.get('/regstatus?reg_id=1')
    assert response.status == 200
    assert response.text == '0'
