(function() {
    /* ================== CONFIG ===================== */

    // let API_URL = 'http://192.168.184.52:8080';
    const API_URL = window.env.API_URL;

    // let G_s_N = 25;
    // let G_b_N = 100;
    let G_s_N = 100;
    let G_b_N = 1000;
    let RESULTS_TOP_SMALL_K = 10;
    let RESULTS_TOP_BIG_K = 1000;
    let job_isEmpty = true;
    // let MIN_NODE_SIZE = 5;
    // let MAX_NODE_SIZE = 15;
    let MIN_NODE_SIZE = 3;
    let MAX_NODE_SIZE = 8;

    let full_names_dictionary = {
        'maxloop': 'K',
        'alpha': 'α',
        'scoring_function': 'Scoring function',
        'source': 'Source',
        'cyclerank': 'Cyclerank',
        'pr': 'PageRank',
        'ssppr': 'Single Source Personalized PageRank',
        '2dr': '2dRank',
        '2drp': 'Single Source 2dRank',
        'cheir': 'CheiRank',
        'cheirp': 'Single Source CheiRank',
    }

    let algorithms_to_numbers = {
        'Cyclerank'                             : 0,
	    'PageRank'                              : 1,
	    'Single Source Personalized PageRank'   : 2,
	    '2dRank'                                : 3,
	    'Single Source 2dRank'                  : 4,
	    'CheiRank'                              : 5,
	    'Single Source CheiRank'                : 6,
	    'Subgraph Generator'                    : 7,
    }

    let viz_g_opt = {
        nodes:{
            color: '#ffffff',
            fixed: false,
            font: {
                // WTF: this option has higher priority then `scaling` and `groups`.
                //      Do not set anything you want dynamic.
                face: '"Varela Round", Helvetica, Arial, sans-serif',
            },
            scaling: {
                label: true
            },
            shadow: true,
            shape: 'dot',
            margin: 10,
            scaling: {
                label: {
                    enabled: false,
                    //min: 14,
                    //max: 24,
                }
            },
        },
        edges: {
            // FIX HERE
            arrows: 'to',
            color: 'black',
            scaling: {
                label: true,
            },
            shadow: true,
        },
        groups: {
            // FIXME: selected nodes make groups invisble
            // FIXME: set groups for this example
            src: {
                color: {
                    background: '#537FE7',
                    highlight: "#537FE7",
                },
                font: {
                    color: '#537FE7',
                }
            }
        }
    }

    /* ================ END CONFIG =================== */


    formEncode = function(data) {
        // https://stackoverflow.com/questions/35325370/how-do-i-post-a-x-www-form-urlencoded-request-using-fetch
        var formBody = [];
        for (var property in data) {
            var encodedKey = encodeURIComponent(property);
            var encodedValue = encodeURIComponent(data[property]);
            formBody.push(encodedKey + "=" + encodedValue);
        }
        return formBody.join("&");
    };


    async function post(url = "", data = {}) {
        // https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API/Using_Fetch
        const response = await fetch(url, {
            method: "POST", // *GET, POST, PUT, DELETE, etc.
            // mode: "cors", // no-cors, *cors, same-origin
            mode: "cors", // no-cors, *cors, same-origin
            cache: "no-cache", // *default, no-cache, reload, force-cache, only-if-cached
            // credentials: "same-origin", // include, *same-origin, omit
            headers: {
                //"Content-Type": "application/json",
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            redirect: "follow", // manual, *follow, error
            referrerPolicy: "no-referrer", // no-referrer, *no-referrer-when-downgrade, origin, origin-when-cross-origin, same-origin, strict-origin, strict-origin-when-cross-origin, unsafe-url
            // body: JSON.stringify(data), // body data type must match "Content-Type" header
            body: formEncode(data),
        });
        return response.json(); // parses JSON response into native JavaScript objects
    };

    async function post_file(url = "", data = {}) {
        // https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API/Using_Fetch
        const response = await fetch(url, {
            method: "POST", // *GET, POST, PUT, DELETE, etc.
            // mode: "cors", // no-cors, *cors, same-origin
            mode: "cors", // no-cors, *cors, same-origin
            cache: "no-cache", // *default, no-cache, reload, force-cache, only-if-cached
            // credentials: "same-origin", // include, *same-origin, omit

            //omitting content type - the browser will fill in this info for us

            redirect: "follow", // manual, *follow, error
            referrerPolicy: "no-referrer", // no-referrer, *no-referrer-when-downgrade, origin, origin-when-cross-origin, same-origin, strict-origin, strict-origin-when-cross-origin, unsafe-url
            // body: JSON.stringify(data), // body data type must match "Content-Type" header
            body: data,
        });
        return response; // return raw response
    };

    async function get(url = "", data = {}) {
        // https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API/Using_Fetch
        const response = await fetch(url + '?' + new URLSearchParams(data), {
            method: "GET", // *GET, POST, PUT, DELETE, etc.
            // mode: "cors", // no-cors, *cors, same-origin
            mode: "cors", // no-cors, *cors, same-origin
            cache: "no-cache", // *default, no-cache, reload, force-cache, only-if-cached
            // credentials: "same-origin", // include, *same-origin, omit
            headers: {
                //"Content-Type": "application/json",
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            redirect: "follow", // manual, *follow, error
            referrerPolicy: "no-referrer", // no-referrer, *no-referrer-when-downgrade, origin, origin-when-cross-origin, same-origin, strict-origin, strict-origin-when-cross-origin, unsafe-url
        });
        return response.json(); // parses JSON response into native JavaScript objects
    };

    let load_datasets = async function() {
        var nameArray = [];
        get(API_URL + '/v0/datasets').then((repo) => {
            var buffer = [];
            var i = 0;
            let bigg = document.getElementById('big-g');
            for (var k in repo) {
                buffer.push(...[
                    '<tr><td><input type="radio" name="dataset", value="',
                    repo[k].path,
                    '" id="ds' + i,
                    '"></td><td>',
                    '<label for="ds' + i + '">',
                    repo[k].name,
                    '</label></td><td id="ds-info' + i + '"class="info_zoom">',
                    repo[k].info,
                    '</td><td>',
                    repo[k].N,
                    '</td><td>',
                    repo[k].M,
                    '</td><td>',                    
                    repo[k].size,
                    '</td>'
                    // '</td><td><a id="ds-info' + i + '" class="nostyle info_zoom" href="#">&#9432;</a></td></tr>',  Use this if you want the reload icon
                    // '</td><td>',
                    // repo[k].info,
                    // '</td>',
                ]);
                nameArray.push(repo[k].info);
                i++;
            }
            let dsbody = document.getElementById('ds-lst-body');
            dsbody.innerHTML = buffer.join('');

            Array.from(document.querySelectorAll('#ds-lst-body tr input'))
                .forEach((e) => {
                    // wiki dataset name here
                    if (e.value == "enwiki.wikigraph.2002-03-01.cleaned.csv") {
                        e.checked = true;
                    }
                });

            window.pre_zoom_scroll_y = window.scrollY;
            // bind info-zoom
            Array.from(document.querySelectorAll('.info_zoom'))
                .forEach((e) => {
                    let id_pos = parseInt(e.id.split('ds-info')[1]);
                    e.addEventListener('click', (a) => {
                    let message = repo[id_pos].info;
    
                    // create holder
                    let buffer = [
                        '<div class="info-hold" id="', id_pos, '-H">',
                            '<div class="info-text">',
                                message,
                            '</div>',
                        '</div>',
                    ];
                    let bigg = document.getElementById('big-g');
                    bigg.innerHTML = bigg.innerHTML + buffer.join('');
            
                    enable_popup(id_pos + '-H');
                })});

            // let r = document.querySelector('#ds-lst-body tr:last-child input');
            // r.checked = true;

            sortTable(1);
        });
    };

    let make_uuid = () => {
        var dt = new Date().getTime();
        var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
            var r = (dt + Math.random()*16)%16 | 0;
            dt = Math.floor(dt/16);
            return (c=='x' ? r :(r&0x3|0x8)).toString(16);
        });
        return uuid;
    }

    let new_job = () => {
        let id = make_uuid();
        window.jobs = window.jobs || {};
        window.jobs[id] = {'id': id, 'tasks': []};
        return id;
    };

    let get_current_job = () => {
        window.jobs = window.jobs || {};
        if (window.current_job === undefined)
            window.current_job = new_job();
        let cj = window.current_job;
        return window.jobs[cj];
    }

    let clean_job = () => {
        window.current_job = undefined;
        return get_current_job();
    }

    let add_to_compare = function(fd) {
        let job = get_current_job();

        /* ----- build job ----- */
        let payload = {
            "job_id": job.id,
            "file": fd.get('dataset'),
            "source": fd.get('source'),
            // "debug"
            // "verbose"
        }

        if (fd.get('dataset') === undefined)
            alert("Please select the dataset.");

        /* -- process each selected algorithm -- */
        var tasks = [];

        /* algorithms' form encoding:
         *   checkbox -> a$api_path
         *   input -> a$api_path:field_name
         */
        let keys = Array.from(fd.keys());
        let algos = keys
            .filter(name => name.startsWith('a$') && name.indexOf(':') == -1);
        let params = keys
            .filter(name => name.startsWith('a$') && name.indexOf(':') != -1);

        console.log("Algos:", algos);
        console.log("Params:", params);

        for (const algo of algos) {
            let algo_clean = algo.split('$')[1];
            let endpoint = API_URL + '/v0/' + algo_clean;
            let data = Object.assign({}, payload);
            params
                .filter(k => k.startsWith(algo))
                .map(k => [k.split(':')[1], fd.get(k)])
                .forEach(kv => data[kv[0]] = kv[1]);
            console.log("Adding", algo_clean, "with", data);
            tasks.push({
                algo: algo_clean,
                endpoint: endpoint,
                data: data,
            });
        }
        job.tasks = job.tasks.concat(tasks);

        /* refresh job view */
        refresh_job_view();
    };

    let refresh_job_view = () => {
        let job = get_current_job();

        var buffer = [
            '<p class="f-h">',
                '<small>Comparison id: ', job.id, '</small>',
                '<h3><a href="#" id="reset-job" class="nostyle">&#128465;</a></h3>',
            '</p>',
            '<table>',
            '<thead><tr><th colspan=2></th><th>dataset</th>',
            '<th>Algorithm</th><th>Source</th><th colspan=100>Parameters</th></thead></tr>'
        ];

        for (i in job.tasks) {
            job_isEmpty = false;
            let t = job.tasks[i];
            
            let algo_full = t.algo;
            if (t.algo in full_names_dictionary)
                algo_full = full_names_dictionary[t.algo];

            buffer.push(...[
                '<tr>',
                '<td>', '' + i, '</td>',
                '<td><a href="#" class="rmq nostyle" data-qid="', '' + i, '">X</a></td>',
                '<td>', t.data['file'], '</td>',
                '<td>', algo_full, '</td>',
                '<td>', t.data['source'], '</td>',
            ]);
            for (k in t.data) {
                if (['file', 'source', 'job_id'].indexOf(k) >= 0)
                    continue;
                let data_name = k;
                if (k in full_names_dictionary)
                    data_name = full_names_dictionary[k];
                buffer.push(...[
                    '<td><em>', data_name, ':</em> ', t.data[k], '</td>',
                ]);
            }
            buffer.push('</tr>');
        }
        buffer.push('</table>');

        let div = document.getElementById('queries');
        div.innerHTML = buffer.join('');

        Array.from(document.querySelectorAll('.rmq'))
            .forEach((e) => e.addEventListener('click', job_rmq));
    };

    let job_rmq = (e) => {
        let job = get_current_job();
        let qid = parseInt(e.target.dataset.qid);
        job.tasks = job.tasks.slice(0, qid).concat(job.tasks.slice(qid+1));
        refresh_job_view();
    };


    let burn_baby_burn = async function() {
        if (window.current_job === undefined || !window.jobs[window.current_job].tasks || job_isEmpty) {
            alert("Please select tasks to compare.");
            return;
        }
        let job = get_current_job();
        var ctx = {'job': job};
        // NOTE: from here on we cannot assume we the current job anymore.

        // sort jobs by algorithm
        job.tasks.sort(function(x,y) {
            // console.log("x:", x, " ", x.algo, " ", algorithms_to_numbers[x.algo]);
            // console.log("y:", y, " ", y.algo, " ", algorithms_to_numbers[y.algo]);
            if (algorithms_to_numbers[full_names_dictionary[x.algo]] < algorithms_to_numbers[full_names_dictionary[y.algo]]) {
                return -1;
            }
            if (algorithms_to_numbers[full_names_dictionary[x.algo]] > algorithms_to_numbers[full_names_dictionary[y.algo]]) {
                return 1;
            }
            return 0;
        });

        var ts = [];
        for (const j of job.tasks) {
            console.log("Invoking", j.algo, "with", j.data, 'src', j.src);
            console.log('full j', j);
            await post(j.endpoint, j.data).then(t => {
                t.src = j.data.source;
                ts.push(t)
            });
        }
        // rimuovere questo per evitare il delete di main premendo sul permalink
        document.getElementById("main").remove();
        wait_for_tasks(ts, ctx);

        // Refresh job id without purging queries
        // if (window.current_job === job.id) {
        //     let j = window.jobs[new_job()];
        //     j.tasks = dcopy(job.tasks);
        //     window.current_job = j.id;
        // }
        // refresh_job_view();
    };

    let status_polling_blocking = async (task_ids) => {
        console.log("Waiting for the following tasks to finish", task_ids);
        var working;
        do  {
            working = false;
            let ps = await fetch(API_URL + '/v0/status').then(r => r.json());
            console.log("-> ids", task_ids);
            console.log("-> ps", ps);
            for (id of task_ids) {
                if (id in ps) {
                    working = true;
                    await new Promise(x => setTimeout(x,  500));
                    break;
                }
            }
        } while (working);
        console.log("The following tasks have completed", task_ids);
    };

    let wait_for_tasks = async function (tasks, ctx) {
        let spinner = document.getElementById("wait");
        spinner.style.display = "block";

        // NOTE: status/ returns list of running tasks !
        //       can use to show progress / dashboard.
        let task_ids = tasks.map(t => t.id);
        await status_polling_blocking(task_ids);

        spinner.style.display = "none";
        load_results(tasks, ctx);
    };

    let permalink = jobid => {
        // This following version required redirect from /
        //document.baseURI.substr(0, document.baseURI.lastIndexOf('/')),
        //'?cmpid=',
        //encodeURIComponent(jobid),

        // the following shall always work.
        return document.baseURI.substr(0, document.baseURI.lastIndexOf('?')) +
                '?cmpid=' + encodeURIComponent(jobid);
    }

    let spinner_code = '<div class="lds-roller"><div></div><div></div><div></div><div></div><div></div><div></div><div></div><div></div></div>';

    let load_results = async function(tasks, ctx) {
        // NOTE: the firs node must be create manually and not via innerHTML
        //       otherwise we will invalidate/reset the graph object
        //       and thus loose the charts.

        if (!tasks) return;
        let jobid = ctx.job.id;
        var buffer = [
            // FIXME: we may want to have a "date" here too
            '<p class="t job-t">',
            'Comparison permalink: ',
            '<a href="',
            permalink(jobid),
            '" target="_blank">',
            jobid,
            '</a>',
            '</p><br><br>',
        ];
        var async_callbacks = [];
        let spinner = document.getElementById("wait_results");
        spinner.style.display = "block";

        for (const t of tasks) {
            let b = await get(API_URL + '/v0/output', {'id': t.id, 'limit': RESULTS_TOP_SMALL_K});

            buffer.push(...[
                '<div class="result" data-tid="', t.id ,'"><div class="sg-container">',
                '<p class="t">Parameters:</p>',
                '<table class="rqparam">',
                    '<tr><td>',
                        'Algorihtm',
                    '</td><td>',
                        b.params.algorithm,
                    '</td></tr>',
                    '<tr><td>',
                        'Dataset',
                    '</td><td>',
                        b.params.file.substr(b.params.file.lastIndexOf("/")+1),
                    '</td></tr>',
                    '<tr><td>',
                        b.params.source ? 'Source' : '',
                    '</td><td>',
                        b.params.source || '&nbsp;',
                    '</td></tr>',
            ]);

            for (p in b.params){
                if (['algorithm', 'file', 'source'].indexOf(p) < 0){
                    let data_name = p;
                    if (p in full_names_dictionary)
                        data_name = full_names_dictionary[p];
                    buffer.push(...[
                        '<tr><td>',
                        data_name,
                        '</td><td>',
                        b.params[p],
                        '</td></tr>',
                    ]);
                }
            }
            buffer.push('</table>');

            buffer.push(...[
                '<p class="t">Ranking:</p>',
                '<div class="restbl_div">',
                '<table class="restbl"><thead><tr>',
                '<th></th>',
                '<th>ID</th>',
                '<th>Label</th>',
                '<th>Score</th>',
                '</tr></thead>',
            ]);
            if (!b.ranking) {
                buffer.push(...[
                    '</table></div>',
                    '<span class="err">Query failed, see the ',
                    '<a href="',
                    API_URL + '/v0/log?id=',
                    t.id, // should be the same as b.task
                    '" target="_blank">logs</a> for more info.',
                ]);
            } else {
                window.query_cache[t.id] = b;
                for (j in b.ranking) {
                    if (j >= RESULTS_TOP_SMALL_K)
                        break;

                    let l = b.ranking[j];
                    buffer.push(...[
                        '<tr><td>#',
                        ''+(parseInt(j)+1),
                        '</td><td>',
                        l.id,
                        '</td><td>',
                        l.label,
                        '</td><td>',
                        // l.score.toFixed(3),
                        l.score.toExponential(3),
                        '</td></tr>'
                    ]);
                }
                buffer.push('</table></div>');

                // async let's load the graph
                let sgid = 'sg$' + t.id;
                buffer.push(...[
                    '<div class="sg-plus-zoom"><div class="sg" id="', sgid, '">',
                    spinner_code,
                    '</div>',
                    '<center><a href="#" class="zoom" data-qid="', t.id, '" ',
                    'data-src="', t.src || b.params.source, '" ',
                    'data-jobid="', jobid,
                    '">expand</a></center>',
                ]);
                async_callbacks.push(() => load_graph(t.id, jobid, G_s_N, sgid, t.src || b.params.source, {
                    'clickToUse': true,
                }));
            }
            buffer.push('</div></div></div>');
        }

        spinner.style.display = "none";

        let o = document.getElementById('output');
        const sec = document.createElement('section')
        sec.classList.add('result_group');
        if (o.childNodes)
            o.insertBefore(sec, o.childNodes[0]);
        else
            o.appendChild(sec);
        sec.innerHTML = buffer.join('');
        sec.scrollIntoView();

        // bind graph-zoom
        Array.from(document.querySelectorAll('.zoom'))
            .forEach((e) => e.addEventListener('click', graph_zoom));

        // now that all graph divs exist, draw graphs!
        async_callbacks.forEach(f => f());

        // change URL
        // Current URL: https://my-website.com/page_a
        const nextURL = permalink(jobid);
        const nextTitle = 'Cyclerank demo :: compare';
        const nextState = { additionalInformation: 'Updated the URL with JS' };

        // This will create a new entry in the browser's history, without reloading
        window.history.pushState(nextState, nextTitle, nextURL);
    };

    let load_graph = async (tid, jobid, n, div_id, src, opts) => {
        // TODO: pass also jobid
        console.log("Checking if subgraph for job_id", jobid, ", query id ", tid, "and maxnodes ", n, " already exists");
        // this API will return all available jobs
        let cmps = await get(API_URL + '/v0/available');
        let job = cmps[jobid];

        // TODO: ensure it completed just like in load_job
        // let task_ids = tasks.map(t => t.id);
        // await status_polling_blocking(task_ids);
        // console.log("job", job);
        let subgraph_exists = false;
        var existing_id;
        var existing_dataset;

        existing_dataset = job.find(e => e.task == tid).params.file;

        for(task in job){
            // console.log("Checking task", task, "with algorithm", job[task].params.algorithm, "and query_to_analyze", job[task].params.query_to_analyze, "and maxnodes", job[task].params.maxnodes);
            if (job[task].params.algorithm == 'Subgraph Generator' && job[task].params.query_to_analyze.includes(tid) && job[task].params.maxnodes == n){
                console.log("Subgraph for", tid, "already exists, will use that: ", job[task]);
                subgraph_exists = true;
                existing_id = job[task].task;
                break;
            }
        }

        if(!subgraph_exists){
            post(API_URL + '/v0/subgraph', {'query_to_analyze': tid, 'maxnodes': n, 'job_id': jobid}) .then(async b => {
                console.log("Subgraph for", tid, "being produced as ", b.id);
                await status_polling_blocking([b.id]);
                console.log("Subgraph created for", tid);
                get(API_URL + '/v0/outputSubgraph', {'id': b.id}).then(bg => {
                    console.log("Loaded graph for", tid, "will create in", div_id);
                    render_graph(bg, div_id, existing_dataset, src, opts);
                });
            });
        }
        else{
            get(API_URL + '/v0/outputSubgraph', {'id': existing_id}).then(bg => {
                console.log("Loaded graph for", tid, "will create in", div_id);
                render_graph(bg, div_id, existing_dataset, src, opts);
            });
        }
    
    };

    let dcopy = o => JSON.parse(JSON.stringify(o)) ;

    let render_graph_vis = (g, div_id, dataset, src, opts) => {
        console.log('Rendering the following graph', g, 'to', div_id);
        let nodes = g.nodes.map(n => {
            let m = n;
            m.value = m.score;
            if (m.label == src)
                m.group = 'src';
            return m
        });

        /* remove spinner */
        document.getElementById(div_id).innerHTML = '';

        opts = Object.assign({}, viz_g_opt, opts || {});
        (function (div, n, e, o) {
            new vis.Network(document.getElementById(div), {
                nodes: n,
                edges: e,
            }, o);
        })(div_id, dcopy(nodes), dcopy(g.edges), dcopy(opts));
    };

    // _sigma
    let render_graph = (g, div_id, dataset, src, opts) => {
        console.log('Rendering the following graph', g, 'to', div_id);
        const graph = new graphology.Graph({multi: true});

        let source_node_id = 0;

        //FIXME: change outputSubgraph instead of remapping 
        let new_nodes = g.nodes.map(n => {
            let m = {
                key: n.id,
                attributes: {
                    label: n.label,
                    score: n.score,
                    // color: "#00ccff",
                    color: g.clusters[n.id],
                    size: n.score*7
                }
            };
            // console.log("cluster: ", g.clusters);
            // if(m.attributes.label == src) {
            //     m.attributes.color = "blue";
            // }
            return m
        });
        let new_edges = g.edges.map(e => {
            // extract the numerical values from the rgb cluster color string
            let cluster_rgb = g.clusters[e.from].substr(4).split(")")[0].split(",");
            // convert rgb values to a darker shade
            let cluster_rgb_shade = cluster_rgb.map(e => Math.floor(e * (1-0.5)));
            let f = {
                source: e.from,
                target: e.to,
                attributes: {
                    size: 0.35,
                    color: "rgb(" + cluster_rgb_shade[0] + "," + cluster_rgb_shade[1] + "," + cluster_rgb_shade[2] + ")"
                }
            };
            // FIXME: find optimal label size for each type of subgraph
            if(parseInt(g.maxnodes) === G_b_N) {
                f.size = 0.35;
            }
            return f;
        });

        //TODO: add custom name and various other customization options (clusters?, colours?)
        graph.import({
            attributes: {name: g.task, dataset: dataset},
            nodes: new_nodes,
            edges: new_edges
        });

        const scores = graph.nodes().map((node) => graph.getNodeAttribute(node, "score"));
        const minDegree = Math.min(...scores);
        const maxDegree = Math.max(...scores);
        graph.forEachNode((node) =>
          graph.setNodeAttribute(
            node,
            "size",
            ((graph.getNodeAttribute(node, "score") - minDegree) / (maxDegree - minDegree)) *
              (MAX_NODE_SIZE - MIN_NODE_SIZE) +
              MIN_NODE_SIZE,
          ),
        );
        
        graphologyLibrary.layout.circular.assign(graph);
        let settings = graphologyLibrary.layoutForceAtlas2.inferSettings(graph);
        settings.gravity = 0.5;
        graphologyLibrary.layoutForceAtlas2.assign(graph, { 
            settings,
            iterations: 400 });
        
        document.getElementById(div_id).innerHTML = '';

        draw_sigma(graph, div_id);
    };

    let graph_zoom = (e) => {
        console.log('event', e);
        let qid = e.target.dataset.qid;
        let src = e.target.dataset.src;
        let jobid = e.target.dataset.jobid;
        let div_id = "fg-" + qid;

        console.log('qid', qid, 'src', src, 'div', div_id);
        window.pre_zoom_scroll_y = window.scrollY;

        // Check if we can just show
        let div = document.getElementById(div_id + '-H');
        if (div) {
            // if we use display hidden/block the graph will be destroyed (only for vis js)
            console.log('showing hidden graph');
            div.style.display = 'block';
            // div.style.left = '0vw';
            return;
        }

        let spinner = document.getElementById("wait");
        spinner.style.display = "block";

        var b = [];
        get(API_URL + '/v0/output', {'id': qid, 'limit': RESULTS_TOP_BIG_K}) .then(async results => {
            
            spinner.style.display = "none";

            b = results;
        
            var info_buffer = [
                '<p>Algorihtm: ',
                b.params.algorithm,
                    ', Dataset: ',
                            b.params.file.substr(b.params.file.lastIndexOf("/")+1),
                            // b.params.source ? ' Source' : ''
            ];
    
            for (p in b.params){
                // if (['algorithm', 'file', 'source'].indexOf(p) < 0){
                if (['algorithm', 'file'].indexOf(p) < 0){
                    let data_name = p;
                    if (p in full_names_dictionary)
                        data_name = full_names_dictionary[p];
    
                    info_buffer.push(...[
                        ', ',
                        data_name,
                        ': ',
                        b.params[p]
                    ]);
                }
            }
    
            info_buffer.push('</p>');
            
    
            var ranking_buffer = [];
            ranking_buffer.push(...[
                // '<p class="t">Ranking:</p>',
                '<div class="restbl_div_big">',
                '<table class="restbl"><thead><tr>',
                '<th></th>',
                '<th>ID</th>',
                '<th>Label</th>',
                '<th>Score</th>',
                '</tr></thead>',
            ]);
            if (!b.ranking) {
                ranking_buffer.push(...[
                    '</table></div>',
                    '<span class="err">Query failed, see the ',
                    '<a href="',
                    API_URL + '/v0/log?id=',
                    qid, // should be the same as b.task
                    '" target="_blank">logs</a> for more info.',
                ]);
            } else {
                for (j in b.ranking) {
                    let l = b.ranking[j];
                    ranking_buffer.push(...[
                        '<tr id="', div_id, l.label.replaceAll(" ", "_"), '"><td>#',
                        ''+(parseInt(j)+1),
                        '</td><td>',
                        l.id,
                        '</td><td>',
                        l.label,
                        '</td><td>',
                        // l.score.toFixed(3),
                        l.score.toExponential(5),
                        '</td></tr>'
                    ]);
                }
                ranking_buffer.push('</table></div>');
            }            
    
            // create holder
            let buffer = [
                '<div class="sg-hold" id="', div_id, '-H">',
                    '<div class="sg-white">',
                        '<div class="sg-info">',
                            ... info_buffer,
                        '</div>',
    
                        '<div class="sg-ranking">',
                            ... ranking_buffer,
                        '</div>',
    
                        '<div class="sg-full" id="', div_id, '">',
                            spinner_code,
                        '</div>',
    
                        '<div class="sg-search" id="', div_id, '-S">',
                          '<input type="search" class="sg-search-input" id="', div_id, '-SI" list="', div_id, '-SD" placeholder="Try searching for a node..." />',
                          '<datalist id="', div_id, '-SD"></datalist>',
                        '</div>',
    
                    '</div>',
                '</div>',
            ];
            let bigg = document.getElementById('big-g');
            bigg.innerHTML = bigg.innerHTML + buffer.join('');
    
            enable_popup(div_id + '-H');
    
    
            // load big graph in it.
            load_graph(qid, jobid, G_b_N, div_id, src);
        
        });
        // let b = window.query_cache[qid];

        
    }

    let enable_popup = (div_id) => {
        let elm = document.getElementById(div_id);
        if (!elm) return;

        document.onkeydown = function(evt) {
            evt = evt || window.event;
            var isEscape = false;
            if ("key" in evt) {
                isEscape = (evt.key === "Escape" || evt.key === "Esc");
            } else {
                isEscape = (evt.keyCode === 27);
            }
            if (isEscape) {
                console.log('esc pressed');
                // if we use display hidden/block the graph will be destroyed  (only for vis js)
                elm.style.display = 'none';
                // elm.remove();
                window.scroll(0, window.pre_zoom_scroll_y || 0);
            }
        };

        // bind click on black
        elm.addEventListener('click', (e) => {
            // if we use display hidden/block the graph will be destroyed (only for vis js)
            //e.target.style.left = '-100vw';
            if (e.target.id == div_id) {
                elm.style.display = 'none'; 
                // e.target.remove();
                window.scroll(0, window.pre_zoom_scroll_y || 0);
             }
        });
    };

    let load_job = async job_id => {
        document.getElementById("main").remove();
        console.log("Loading foreign job_id", job_id);
        // this API will return all available jobs
        let cmps = await get(API_URL + '/v0/available');
        // check if we have it
        if (!(job_id in cmps))
            return alert('Comparison not found (' + job_id + ')');
        // ensure it completed
        let job = cmps[job_id];

        console.log("Unsorted job: ", job);
        // sort jobs by algorithm
        job.sort(function(x,y) {
            // console.log("x:", x, " ", x.params.algorithm, " ", algorithms_to_numbers[x.params.algorithm]);
            // console.log("y:", y, " ", y.params.algorithm, " ", algorithms_to_numbers[y.params.algorithm]);
            if (algorithms_to_numbers[x.params.algorithm] < algorithms_to_numbers[y.params.algorithm]) {
                return -1;
            }
            if (algorithms_to_numbers[x.params.algorithm] > algorithms_to_numbers[y.params.algorithm]) {
                return 1;
            }
            return 0;
        });
        console.log("Sorted job: ", job);

        // ignore subgraph generator
        var ts = [];
        for (const j of job){
            console.log("job j is ", j);
            if(j.params.algorithm === 'Subgraph Generator') continue;
            ts.push({'id': j.task});
        }
        wait_for_tasks(ts, {'job': {'id': job_id}});
    };
    window.load_job = load_job;

    let urltoid = () => {
        const queryString = window.location.search;
        const urlParams = new URLSearchParams(queryString);
        return urlParams.get('cmpid');
    }

    let autoload_job = () => {
        let jobid = urltoid();
        if (jobid) load_job(jobid);
    }

    let load_history = async () => {
        let cmps = await get(API_URL + '/v0/available');
        let buffer = [
            '<thead><tr><th></th><th>Dataset</th><th>Algorithm</th>',
            '<th>Source</th><th colspan=100>Parameters</th></tr></thead>',
        ];
        for (j in cmps) {
            if (j == '00000000-0000-0000-0000-000000000000') // ???
                continue;
            if (!cmps[j]) continue; // skip empty
            buffer.push(...[
                '<tr class="job">',
                    //'<td><a href="#" class="load-me", data-jid="', j, '">load</a></td>',
                    //'<td colspan="100"><a href="',permalink(j),'" target=_blank>', j, '</a></td>',
                    // '<td colspan="100" id="permalink"><a href="#" class="load-me", data-jid="', j, '">load</a><span>, </span><a href="',permalink(j),'" target=_blank>permalink</a></td>',
                    '<td colspan="100" id="permalink"><a href="',permalink(j),'" target=_blank>permalink</a></td>',
                '</tr>',
            ]);

            for (t of cmps[j]) {
                if(t.params.algorithm == 'Subgraph Generator') continue;
                buffer.push(...[
                    '<tr>',
                        '<td></td>',
                        '<td>',
                            t.params.file.substr(t.params.file.lastIndexOf('/')+1),
                        '</td>',
                        '<td>', t.params.algorithm, '</td>',
                        '<td>', t.params.source || '', '</td>',
                ]);
                for (p in t.params) {
                    if (['algorithm', 'file', 'source'].indexOf(p) < 0)
                        buffer.push(...[
                            '<td><em>', p, '</em>: ', t.params[p], '</td>',
                        ]);
                }
                buffer.push('</tr>');
            }
        }
        document.getElementById('history').innerHTML = buffer.join('');

        Array.from(document.querySelectorAll('.load-me'))
            .forEach((a) => a.addEventListener('click',
                (e) => {
                    e.preventDefault();
                    load_job(e.target.dataset.jid);
                }));
    }

    let show_upload_datasets = (e) => {
        console.log('event', e);
        window.pre_zoom_scroll_y = window.scrollY;

        // Check if we can just show
        let div = document.getElementById('upload-datasets');
        if (div) {
            console.log('showing hidden upload datasets');
            div.style.display = 'flex';
            document.getElementById('upload-result').style.display = 'none';
            enable_popup('upload-datasets');
            return;
        }
    }

    /* ---- https://developer.mozilla.org/en-US/docs/Web/API/HTMLFormElement/formdata_event */
    const exec_f = document.getElementById("exec_f");
    exec_f.addEventListener("submit", (e) => {
        e.preventDefault();
        const formData = new FormData(exec_f);
        add_to_compare(formData);
    });

    const burn = document.getElementById("submit_job");
    burn.addEventListener("click", (e) => {
        e.preventDefault();
        burn_baby_burn();
    });

    const ds_reload = document.getElementById("ds-reload");
    ds_reload.addEventListener("click", (e) => {
        e.preventDefault();
        load_datasets();
    });
    
    const ds_upload = document.getElementById("ds-upload");
    ds_upload.addEventListener("click", (e) => {
        show_upload_datasets();
    });

    const queries = document.getElementById("queries");
    queries.addEventListener("click", (e) => {
        if (!e.target.id || e.target.id !== 'reset-job')
            return
        job_isEmpty = true;
        e.preventDefault();
        clean_job();
        refresh_job_view();
    });
    
    let wait_for_upload = async function (formData) {
        
        // document.getElementById("upload-datasets").style.display = 'none';
        window.scroll(0, window.pre_zoom_scroll_y || 0);

        let spinner = document.getElementById("wait");
        spinner.style.display = "block";
        
        console.log("attempting upload for ", formData);
        await post_file(window.env.API_URL + "/v0/upload", formData).then(e => {
            spinner.style.display = "none";
            console.log("response: ", e);
            // alert(e.statusText);
            let upload_result_string = "";
            switch (e.status) {
                case 405:
                    upload_result_string = "Wrong method, correct method is POST"
                    break;
                case 400:
                    upload_result_string = "Please insert a file in the form"
                    break;
                case 422:
                    upload_result_string = "File either already exists, exceeds 1Gb limit or has unsafe name"
                    break;
                case 500:
                    upload_result_string = "Internal error"
                    break;
                case 200:
                    upload_result_string = "Upload accepted"
                    break;
            
                default:
                    upload_result_string = "Unknown response from server"
                    break;
            }
            document.getElementById("upload-result").innerHTML = upload_result_string;
            document.getElementById("upload-result").style.display = 'inline-block';
        })

    };
    
    const upload_form = document.getElementById("upload_form");
    upload_form.addEventListener("submit", (e) => {
        e.preventDefault();
        const formData = new FormData(upload_form);
        wait_for_upload(formData);
        // location.reload();
    });

    document.getElementById("load-pre").addEventListener("click", load_history);

    window.query_cache = {};
    let jobid = urltoid();
    if (jobid)
        autoload_job();
    else {
        load_datasets();
        load_history();
    }
    // TODO: if job id in GET, then load it's results (conditionally to wait).
})();

// sort table on th press https://www.w3schools.com/howto/howto_js_sort_table.asp
function sortTable(n) {
    console.log("Sorting ds-lst by th number", n);
    var table, rows, switching, i, x, y, shouldSwitch, dir, switchcount = 0;
    table = document.getElementById("ds-lst");
    switching = true;
    // Set the sorting direction to ascending:
    dir = "asc";
    /* Make a loop that will continue until
    no switching has been done: */
    while (switching) {
      // Start by saying: no switching is done:
      switching = false;
      rows = table.rows;
      /* Loop through all table rows (except the
      first, which contains table headers): */
      for (i = 1; i < (rows.length - 1); i++) {
        // Start by saying there should be no switching:
        shouldSwitch = false;
        /* Get the two elements you want to compare,
        one from current row and one from the next: */
        x = rows[i].getElementsByTagName("TD")[n];
        y = rows[i + 1].getElementsByTagName("TD")[n];

        /* Check if the two rows should switch place,
        based on the direction, asc or desc: */
        if (dir == "asc" && n == 1) {
            if (x.innerHTML.toLowerCase().split(">")[1] > y.innerHTML.toLowerCase().split(">")[1]) {
              // If so, mark as a switch and break the loop:
              shouldSwitch = true;
              break;
            }
        } else if (dir == "desc" && n == 1) {
            if (x.innerHTML.toLowerCase().split(">")[1] < y.innerHTML.toLowerCase().split(">")[1]) {
              // If so, mark as a switch and break the loop:
              shouldSwitch = true;
              break;
            }
        } else if (dir == "asc") {
            if (Number(x.innerHTML.replaceAll(",", "")) > Number(y.innerHTML.replaceAll(",", ""))) {
                shouldSwitch = true;
                break;
            }
        } else if (dir == "desc") {
            if (Number(x.innerHTML.replaceAll(",", "")) < Number(y.innerHTML.replaceAll(",", ""))) {
                shouldSwitch = true;
                break;
            }
        }
      }
      if (shouldSwitch) {
        /* If a switch has been marked, make the switch
        and mark that a switch has been done: */
        rows[i].parentNode.insertBefore(rows[i + 1], rows[i]);
        switching = true;
        // Each time a switch is done, increase this count by 1:
        switchcount ++;
      } else {
        /* If no switching has been done AND the direction is "asc",
        set the direction to "desc" and run the while loop again. */
        if (switchcount == 0 && dir == "asc") {
          dir = "desc";
          switching = true;
        }
      }
    }
    // set all th to unsorted symbols
    var headers = document.querySelectorAll('#ds-lst thead tr th');
    for (i = 0; i < headers.length; i++) {
        console.log("header was ", headers[i].innerHTML);
        var header =  headers[i].innerHTML.replace("▴", "");
        header = header.replace("▾", "");
        if ((i==1 || i==3 || i==4 || i==5) && i != n) {
            header = header + "&#x25b4;&#x25be;";
        } else if (i == n) {
            if (dir == "asc") {                
                header = header + "&#x25be;";
            } else {
                header = header + "&#x25b4;";
            }
        }
        console.log("header is ", header);
        headers[i].innerHTML = header;
    }
}

function drawLabel(context, data, settings) {
    if (!data.label) return;
  
    const size = settings.labelSize,
    font = settings.labelFont,
    weight = settings.labelWeight;
  
    context.font = `${weight} ${size}px ${font}`;
    const width = context.measureText(data.label).width + 8;
  
    // context.fillStyle = "#ffffff11";
    // context.fillRect(data.x + data.size, data.y + size / 3 - 15, width, 20);
    
    var style = getComputedStyle(document.body);
    if (localStorage.getItem('theme') === 'theme-light') {
        context.fillStyle = "#ffffff99";
        context.fillRect(data.x + data.size, data.y + size / 3 - 15, width, 20);
        context.fillStyle = style.getPropertyValue('--grey-color');
    }
    else {
        context.fillStyle = style.getPropertyValue('--grey-color');
    }

    context.fillText(data.label, data.x + data.size + 3, data.y + size / 3);
}

function draw_sigma(graph, div_id) {
    var settings;
    settings = {
        // defaultEdgeColor: "#999999",
        defaultEdgeType: "arrow",
        
        labelSize: 12,
        labelColor: { color: "black" },

        HoverLabelBGColor: "#002147",
        labelBGColor: "#ddd",
        labelHoverColor: "black",
        hoverFontStyle: "bold",
        labelDensity: 0.07,
        labelGridCellSize: 60,
        labelRenderer: drawLabel,
        labelRenderedSizeThreshold: 1,

        zIndex: true,

        hideEdgesOnMove: true,
        hideLabelsOnMove: false,
        // allowInvalidContainer: true,
        // maxEdgeSize: 0.5,
        // minEdgeSize: 0.2,
        // minCameraRatio: 0.75, // How far can we zoom out?
        // maxCameraRatio: 20, // How far can we zoom in?
    };

    sigma = new Sigma(graph, document.getElementById(div_id), settings);
    // var sigma = new Sigma(graph, document.getElementById(div_id));

    addListeners(sigma, graph, div_id);
}

function addListeners(sigma, graph, div_id) {
    var hoveredNode;
    var hoveredNeighbors;
    var selectedNode;
    var searchQuery = "";
    var suggestions;
    var tablerow;

    const searchInput = document.getElementById(div_id + "-SI");
    const searchSuggestions = document.getElementById(div_id + "-SD");

    if(searchInput) { 
        // console.log("searchInput found");
        // Feed the datalist autocomplete values:
        searchSuggestions.innerHTML = graph
        .nodes()
        .map((node) => `<option value="${graph.getNodeAttribute(node, "label")}"></option>`)
        .join("\n");
        
        // Bind search input interactions:
        searchInput.addEventListener("input", () => {
        //   console.log("searchInput input");
          setSearchQuery(searchInput.value || "");
        });
        searchInput.addEventListener("blur", () => {
        //   console.log("searchInput blur");
          setSearchQuery("");
        });
    }

    sigma.on("clickNode", ({ node }) => {
        console.log("clickNode", graph.getNodeAttribute(node, "label") );
        if(graph.getAttribute('dataset').includes("wiki.wikigraph")) {
            var url = "https://" + graph.getAttribute('dataset').slice(1, 3) + ".wikipedia.org/wiki/" + graph.getNodeAttribute(node, "label").replaceAll(" ", "_");
            console.log("Opening window ", url);
            window.open(url, '_blank');
        }
    });
    sigma.on("enterNode", ({ node }) => {
        console.log("enterNode", node );
        setHoveredNode(node);
        document.getElementById(div_id).classList.add("nodes-pointer");
    });
    sigma.on("leaveNode", () => {
        console.log("leaveNode");
        setHoveredNode(undefined);
        document.getElementById(div_id).classList.remove("nodes-pointer");
    });

    function setSearchQuery(query) {
        searchQuery = query;

        if (searchInput.value !== query) searchInput.value = query;

        if (query) {
            const lcQuery = query.toLowerCase();
            const new_suggestions = graph
            .nodes()
            .map((n) => ({ id: n, label: graph.getNodeAttribute(n, "label") }))
            .filter(({ label }) => label.toLowerCase().includes(lcQuery));

            // If we have a single perfect match, them we remove the suggestions, and
            // we consider the user has selected a node through the datalist
            // autocomplete:
            if (new_suggestions.length === 1 && new_suggestions[0].label === query) {
                selectedNode = new_suggestions[0].id;
                suggestions = undefined;

                // Move to node's table row:
                tablerow = document.getElementById(div_id + query.replaceAll(" ", "_"));
                tablerow.scrollIntoView({ behavior: 'smooth', block: 'center' });
                tablerow.classList.add("highlight");

                // Move the camera to center it on the selected node:
                const nodePosition = sigma.getNodeDisplayData(selectedNode);
                sigma.getCamera().animate(nodePosition, {
                    duration: 500,
                });
            }
            // Else, we display the suggestions list:
            else {
                console.log("new_suggestions", new_suggestions);
                selectedNode = undefined;
                suggestions = new Set(new_suggestions.map(({ id }) => id));
            }
        }
        // If the query is empty, then we reset the selectedNode / suggestions state:
        else {
            selectedNode = undefined;
            suggestions = undefined;
            tablerow.classList.remove("highlight");
        }

        // Refresh rendering:
        sigma.refresh();
    }

    function setHoveredNode(node) {
        if (node) {
          hoveredNode = node;
          hoveredNeighbors = new Set(graph.neighbors(node));
        } else {
          hoveredNode = undefined;
          hoveredNeighbors = undefined;
        }
      
        // Refresh rendering:
        sigma.refresh();
    }

    sigma.setSetting("nodeReducer", (node, data) => {
        const res = { ...data };
      
        if (hoveredNeighbors && !hoveredNeighbors.has(node) && hoveredNode !== node) {
          res.label = "";
          res.color = "grey";
        }

        if (selectedNode === node) {
          res.highlighted = true;
        } else if (suggestions && !suggestions.has(node)) {
          res.label = "";
          res.color = "grey";
        }
      
        return res;
    });
      
    // Render edges accordingly to the current state:
    // 1. If a node is hovered, the edge is hidden if it is not connected to the
    //    node
    // 2. If there is a query, the edge is only visible if it connects two
    //    suggestions
    sigma.setSetting("edgeReducer", (edge, data) => {
        const res = { ...data };

        if (hoveredNode && !graph.hasExtremity(edge, hoveredNode)) {
          res.hidden = true;
        }
        else if (hoveredNode) {
            res.size = 4;
            // extract the numerical values from the node's rgb color string
            let cluster_rgb = graph.getNodeAttribute(hoveredNode, "color").substr(4).split(")")[0].split(",");
            // convert rgb values to a darker shade
            let cluster_rgb_shade = cluster_rgb.map(e => Math.floor(e * (1-0.5)));
            let color = "rgb(" + cluster_rgb_shade[0] + "," + cluster_rgb_shade[1] + "," + cluster_rgb_shade[2] + ")";

            res.color = color;
        }

        if (suggestions && (!suggestions.has(graph.source(edge)) || !suggestions.has(graph.target(edge)))) {
          res.hidden = true;
        }
        else if (suggestions) {
            res.size = 4;
            // extract the numerical values from the node's rgb color string
            let cluster_rgb = graph.getNodeAttribute(graph.source(edge), "color").substr(4).split(")")[0].split(",");
            // convert rgb values to a darker shade
            let cluster_rgb_shade = cluster_rgb.map(e => Math.floor(e * (1-0.5)));
            let color = "rgb(" + cluster_rgb_shade[0] + "," + cluster_rgb_shade[1] + "," + cluster_rgb_shade[2] + ")";

            res.color = color;            
        }

      
        return res;
      });
}

function spawnSpinner() {
    let spinner_code = '<div class="lds-roller" id="load"><div></div><div></div><div></div><div></div><div></div><div></div><div></div><div></div></div>';
    
    let upload = document.getElementById("upload_article");
    upload.innerHTML = upload.innerHTML + spinner_code;

    setTimeout(() => { let spinner = document.getElementById("load"); console.log("Spinner"); spinner.remove(); }, 5000);
    
    return;
}
