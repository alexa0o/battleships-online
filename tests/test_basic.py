import time
import json


# Start via `make test-debug` or `make test-release`
async def test_basic(service_client):
    response = await service_client.get('/regnewgame')
    assert response.status == 200
    assert response.text == '0'

    response = await service_client.get('/regnewgame')
    assert response.status == 200
    assert response.text == '1'

    time.sleep(5)

    response = await service_client.get('/regstatus?reg_id=0')
    assert response.status == 200
    assert response.text == '1'

    response = await service_client.get('/regstatus?reg_id=1')
    assert response.status == 200
    assert response.text == '0'

    response = await service_client.post(
            '/sendfield?player_id=0',
            data=json.dumps({'left_field': {
                'field': [
                    [0, 0, 0, 0, 0, 0, 0, 1, 1, 1],
                    [1, 0, 1, 0, 0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                    [1, 1, 0, 0, 0, 0, 0, 1, 0, 1],
                    [0, 0, 0, 0, 1, 0, 0, 1, 0, 0],
                    [0, 0, 0, 0, 1, 0, 0, 1, 0, 0],
                    [0, 1, 0, 0, 1, 0, 0, 1, 0, 1]]}}))
    assert response.status == 200

    response = await service_client.post(
            '/sendfield?player_id=1',
            data=json.dumps({'left_field': {
                'field': [
                    [0, 0, 0, 0, 0, 0, 0, 1, 1, 1],
                    [1, 0, 1, 0, 0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                    [1, 1, 0, 0, 0, 0, 0, 1, 0, 1],
                    [0, 0, 0, 0, 1, 0, 0, 1, 0, 0],
                    [0, 0, 0, 0, 1, 0, 0, 1, 0, 0],
                    [0, 1, 0, 0, 1, 0, 0, 1, 0, 1]]}}))
    assert response.status == 200

    for x in range(10):
        for y in range(10):
            for turn in [1, 0]:
                response = await service_client.get(
                        '/trykill?player_id={player_id}&x={x}&y={y}'.format(
                            player_id=turn, x=x, y=y))
                if x == 0 and y == 0:
                    assert response.text == "Miss"
                elif x == 1 and y == 0:
                    assert response.text == "Kill"
                elif x == 0 and y == 8:
                    assert response.text == "Damage"
                elif x == 0 and y == 9:
                    assert response.text == "Kill"

                assert response.status == 200

    response = await service_client.get('/trykill?player_id=1&x=0&y=0')
    assert response.status == 200
    assert response.text == 'You win'

    response = await service_client.get('/trykill?player_id=0&x=0&y=0')
    assert response.status == 200
    assert response.text == 'You lose'
